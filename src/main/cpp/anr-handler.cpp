/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <dirent.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <agent-ndk.h>

#include "anr-handler.h"
#include "procfs.h"
#include "signal-utils.h"

static pid_t pid = getpid();
static pid_t anr_monitor_tid = -1;
static const char *const ANR_THREAD_NAME = "Signal Catcher";
static const char *const ANR_SIGBLK_LABEL = "SigBlk:\t";
static size_t const ANR_THREAD_NAME_LEN = std::strlen(ANR_THREAD_NAME);
static size_t const ANR_SIGBLK_LABEL_LEN = std::strlen(ANR_SIGBLK_LABEL);
static const uint64_t ANR_THREAD_SIGBLK = 0x1000;

static volatile bool enabled = true;
static volatile bool watchdog_triggered = false;

static sem_t watchdog_semaphore;
static bool watchdog_has_sem = false;

/**
 * https://android.googlesource.com/platform/art/+/master/runtime/signal_catcher.cc
 */

/**
 * Signal the runtime handler using syscall() with SYS_tgkill and
 * SIGQUIT to target the Android handler thread
 */
void raise_anr_signal() {
    if (pid >= 0 && anr_monitor_tid >= 0) {
        _LOGD("raise_anr_signal: pid [%d] tid [%d]", pid, anr_monitor_tid);
        syscall(SYS_tgkill, pid, anr_monitor_tid, SIGQUIT);
    }
}

void *anr_monitor_thread(__unused void *unused) {
    static const useconds_t poll_sleep = 100000;

    _LOGD("anr_monitor_thread: started");
    while (true) {
        watchdog_triggered = false;
        _LOGD("anr_monitor_thread: waiting on trigger via %s",  watchdog_has_sem ? "semaphore" : "polling");
        if (!watchdog_has_sem || sem_wait(&watchdog_semaphore) != 0) {
            while (enabled && !watchdog_triggered) {
                _LOGD("anr_monitor_thread: sleeping [%d] ns", poll_sleep);
                usleep(poll_sleep);
            }
        }

        // Raise SIGQUIT to alert ART's ANR processing
        _LOGD("anr_monitor_thread: raising ANR signal");
        raise_anr_signal();

        if (enabled) {
            _LOGE("Notify the agent an ANR has occurred");
        }

        // Unblock SIGQUIT again so handler will run again.
        sigutils::unblock_signal(SIGQUIT);
    }
    _LOGD("anr_monitor_thread: stopped");

    return nullptr;
}

static void anr_interceptor(__unused int signo, __unused siginfo_t *info, __unused void *user_context) {

    // Block SIGQUIT in this thread so the Google handler can run.
    sigutils::block_signal(SIGQUIT);

    if (enabled) {
        _LOGE("TODO Collect stacktrace from unwinder");
    }

    // set the trigger flag for the poll loop if a semaphore was not created
    watchdog_triggered = true;

    // Signal the ANR monitor thread to report:
    if (watchdog_has_sem && (sem_post(&watchdog_semaphore) != 0)) {
        _LOGE("Could not post ANR handler semaphore");
    }
}


/**
 * THE Android runtime uses a SIGQUIT handler to monitor ANR state, The handler runs
 * on a well-known thread ("Signal Catcher") with a known SIGBLK value (0000000000001000).
 * Scan the /proc tree for the the current process to identify the thread ID.
 */
bool detect_android_anr_handler() {
    struct dirent *_dirent;
    std::string path;
    const char *taskPath = procfs::get_task_path(getpid(), path);
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
        procfs::get_thread_name(tid, threadName);

        if (strncmp(threadName.c_str(), ANR_THREAD_NAME, ANR_THREAD_NAME_LEN) == 0) {
            char buff[1024];
            uint64_t sigblk = 0;
            std::string threadStatus;
            FILE *fp = fopen(procfs::get_thread_status_path(tid, threadStatus), "r");

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
                _LOGD("Android ANR monitor: tid[%d]", anr_monitor_tid);
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
}


/**
 * Detect and co-opt the existing Android ANR monitor
 * @return
 */
bool anr_handler_initialize() {
    pthread_t watchdog_thread;

    _LOGD("anr_handler_initialize: starting");

    // detect the Android runtime's ANR signal handler
    if (!detect_android_anr_handler()) {
        _LOGE("Failed to detect Google ANR monitor thread. ANR report won't be sent to Google.");
    }

    // Start a watchdog thread
    if (pthread_create(&watchdog_thread, nullptr, anr_monitor_thread, nullptr) != 0) {
        _LOGE("Could not create ANR watchdog thread. ANR reports won't be collected.");
        return false;
    }

    // alloc our thread semaphore
    watchdog_has_sem = (sem_init(&watchdog_semaphore, 0, 0) == 0);
    if (!watchdog_has_sem) {
        _LOGW("Failed to init semaphore. Will poll instead");
    }

    // Install the ANR handler
    sigutils::install_handler(SIGQUIT, anr_interceptor);

    // Unblock SIGQUIT to allow the ANR handler to run
    _LOGD("anr_handler_initialize: posting to watchdog sem [%d]", (int) watchdog_has_sem);
    sigutils::unblock_signal(SIGQUIT);

    enabled = true;

    _LOGD("anr_handler_initialize: enabled[%d]", (int) enabled);

    return enabled;
}

/**
 * Disable ANR handling via SIGQUIT
 * @return true
 */
bool anr_handler_shutdown() {
    reset_android_anr_handler();
    if (watchdog_has_sem && (sem_post(&watchdog_semaphore) != 0)) {
        _LOGE("anr_handler_shutdown: Could not unlock ANR handler semaphore");
    }
    sem_close(&watchdog_semaphore);
    enabled = false;
    watchdog_triggered = false;

    return true;
}

