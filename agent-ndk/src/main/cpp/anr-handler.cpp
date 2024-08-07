/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <agent-ndk.h>
#include <errno.h>
#include "procfs.h"
#include "signal-utils.h"
#include "backtrace.h"
#include "serializer.h"
#include "anr-handler.h"


static pid_t pid = getpid();
static pid_t anr_monitor_tid = -1;
static const char *const ANR_THREAD_NAME = "Signal Catcher";
static const char *const ANR_SIGBLK_LABEL = "SigBlk:\t";
static size_t const ANR_THREAD_NAME_LEN = std::strlen(ANR_THREAD_NAME);
static size_t const ANR_SIGBLK_LABEL_LEN = std::strlen(ANR_SIGBLK_LABEL);
static const uint64_t ANR_THREAD_SIGBLK = 0x1000;

static volatile bool enabled = true;
static volatile bool watchdog_triggered = false;

static pthread_t watchdog_thread = 0;
static sem_t watchdog_semaphore;
static bool watchdog_must_poll = false;
static char *reportBuffer = nullptr;


void *anr_monitor_thread(__unused void *unused) {
    static const useconds_t poll_sleep = 100000;

    _LOGD("anr_monitor_thread: started (enabled[%d])", (int) enabled);
    if (0 != pthread_setname_np(pthread_self(), "NR-ANR-Handler")) {
        _LOGE_POSIX("pthread_setname_np()");
    }

    while (enabled) {
        watchdog_triggered = false;

        _LOGD("anr_monitor_thread: waiting on trigger via %s",
              watchdog_must_poll ? "polling" : "semaphore");

        if (watchdog_must_poll || sem_wait(&watchdog_semaphore) != 0) {
            while (enabled && !watchdog_triggered) {
                _LOGD("anr_monitor_thread: sleeping [%d] ns", poll_sleep);
                usleep(poll_sleep);
            }
        }

        if (enabled) {
            // Raise SIGQUIT to alert ART's ANR processing
            _LOGD("anr_monitor_thread: raising ANR signal");
            /*
             * Signal the runtime handler using syscall() with SYS_tgkill and
             * SIGQUIT to target the Android handler thread
             */
            if (pid >= 0 && anr_monitor_tid >= 0) {
                _LOGD("raise_anr_signal: pid [%d] tid [%d]", pid, anr_monitor_tid);
                syscall(SYS_tgkill, pid, anr_monitor_tid, SIGQUIT);
            }
        }

        // Unblock SIGQUIT again so handler will run again.
        sigutils::unblock_signal(SIGQUIT);
    }
    _LOGD("anr_monitor_thread: stopped (enabled[%d])", (int) enabled);

    return nullptr;
}

void anr_interceptor(__unused int signo, siginfo_t *_siginfo, void *ucontext) {

    // Block SIGQUIT in this thread so the default handler can run.
    sigutils::block_signal(SIGQUIT);

    if (enabled) {
        _LOGD("Notify the JVM delegate an ANR has occurred");
        const ucontext_t *_ucontext = static_cast<const ucontext_t *>(ucontext);

        _LOGI("ANR interceptor invoked:");
        if (reportBuffer != nullptr) {
            if (collect_backtrace(reportBuffer, BACKTRACE_SZ_MAX, _siginfo, _ucontext)) {
                serializer::from_anr(reportBuffer, BACKTRACE_SZ_MAX);
                _LOGI("ANR report posted from Android handler");
            }
        } else {
            _LOGE("Buffer not allocated for ANR report!");
        }
    }

    // set the trigger flag for the poll loop if a semaphore was not created
    watchdog_triggered = true;

    // Signal the ANR monitor thread to report:
    if (!watchdog_must_poll && (sem_post(&watchdog_semaphore) != 0)) {
        _LOGE("Could not post ANR handler semaphore");
        watchdog_must_poll = true;
    }
}


/**
 * THE Android runtime uses a SIGQUIT handler to monitor ANR state, The handler runs
 * on a well-known thread ("Signal Catcher") with a known SIGBLK value (0000000000001000).
 * Scan the /proc tree for the the current process to identify the thread ID.
 */
bool detect_android_anr_handler() {
    pid_t pid = getpid();
    struct dirent *_dirent;
    std::string path;
    const char *taskPath = procfs::get_task_path(pid, path);
    DIR *dir = opendir(taskPath);

    // iterate through this process' threads, looking for the ANR monitor thread
    while ((_dirent = readdir(dir)) != nullptr) {
        // only interested in numeric directories (representing thread ids)
        if (!isdigit(_dirent->d_name[0])) {
            continue;
        }

        // convert dir entry name to thread id
        pid_t tid = std::strtol(_dirent->d_name, nullptr, 10);

        std::string threadName;
        procfs::get_thread_name(pid, tid, threadName);

        if (std::strncmp(threadName.c_str(), ANR_THREAD_NAME, ANR_THREAD_NAME_LEN) == 0) {
            char buff[1024];
            uint64_t sigblk = 0;
            std::string threadStatus;
            FILE *fp = fopen(procfs::get_thread_status_path(pid, tid, threadStatus), "r");

            if (fp != nullptr) {
                while (fgets(buff, sizeof(buff), fp) != NULL) {
                    if (std::strncmp(buff, ANR_SIGBLK_LABEL, ANR_SIGBLK_LABEL_LEN) == 0) {
                        sigblk = std::strtoull(buff + ANR_SIGBLK_LABEL_LEN, nullptr, 16);
                        break;
                    }
                }
                fclose(fp);
            }

            if ((sigblk & ANR_THREAD_SIGBLK) == ANR_THREAD_SIGBLK) {
                anr_monitor_tid = tid;
                _LOGD("Android ANR monitor found on thread[%d]", anr_monitor_tid);
            } else {
                _LOGE("Cannot access Android runtime ANR monitor while debugging");
            }
        }

        if (anr_monitor_tid != -1) {
            break;
        }
    }

    closedir(dir);

    return (anr_monitor_tid != -1);
}

/**
 * Return anr detection to previous state
 */
void reset_android_anr_handler() {
    pid = 0;
    anr_monitor_tid = -1;
    if (reportBuffer != nullptr) {
        delete[] reportBuffer;
        reportBuffer = nullptr;
    }
}


/**
 * Detect and co-opt the existing Android ANR monitor
 * @return
 */
bool anr_handler_initialize() {

    // detect the Android runtime's ANR signal handler
    if (!detect_android_anr_handler()) {
        _LOGE("Failed to detect the Android ANR monitor thread. Native ANR reports will not be sent to New Relic.");
    }

    // Start a watchdog thread
    if (0 != pthread_create(&watchdog_thread, nullptr, anr_monitor_thread, nullptr)) {
        _LOGE("Could not create an ANR watchdog thread. ANR reports will not be collected.");
        return false;
    }

    // alloc our thread semaphore
    watchdog_must_poll = (sem_init(&watchdog_semaphore, 0, 0) != 0);
    if (watchdog_must_poll) {
        _LOGW("Failed to init semaphore, revert to polling");
    }

    // Install the new SIGQUIT (ANR) handler. DO NOT CALL the previous SIGQUIT handler
    if (!sigutils::install_handler(SIGQUIT, anr_interceptor, nullptr, 0)) {
        _LOGE("Could not install SIGQUIT handler: ANR reports will not be collected.");
    }

    // Unblock SIGQUIT to allow the ANR handler to run
    sigutils::unblock_signal(SIGQUIT);

    reportBuffer = new char[BACKTRACE_SZ_MAX];
    enabled = true;

    _LOGD("anr_handler_initialize: watchdog sem [%p]", &watchdog_semaphore);

    return enabled;
}

/**
 * Disable ANR handling via SIGQUIT
 */
void anr_handler_shutdown() {
    enabled = false;

    if (!watchdog_must_poll && (sem_post(&watchdog_semaphore) != 0)) {
        _LOGE("anr_handler_shutdown: Could not unlock ANR handler semaphore");
    }

    if (watchdog_thread != 0) {
        if (0 != pthread_join(watchdog_thread, nullptr)) {
            _LOGE_POSIX("pthread_join failed");
        }
        watchdog_thread = (pthread_t) nullptr;
    }

    sem_close(&watchdog_semaphore);
    watchdog_triggered = false;

    reset_android_anr_handler();
}

