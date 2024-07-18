/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>
#include <pthread.h>

#include <agent-ndk.h>
#include <errno.h>
#include "signal-utils.h"
#include "backtrace.h"
#include "serializer.h"
#include "signal-handler.h"

typedef struct observed_signal {
    int signo;
    const char *name;
    const char *description;
    struct sigaction sa_previous;
    volatile sig_atomic_t intercepting;

} observed_signal_t;

/* forward decls */
void invoke_sigaction(int signo, struct sigaction *_sigaction, siginfo_t *_siginfo, void *context);

void invoke_previous_sigaction(int signo, siginfo_t *_siginfo, void *context);

void install_handler();

void uninstall_handler();

observed_signal_t *observed_signal_get_or_null(int);

/* replacement stack */
static stack_t _stack = {};

/* Module-wide mutex */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Collection of observed signals */
static observed_signal_t observedSignals[] = {
        {SIGILL,  "SIGILL",  "Illegal instruction",                               {}, 0},
        {SIGTRAP, "SIGTRAP", "Trap (invalid memory reference)",                   {}, 0},
        {SIGABRT, "SIGABRT", "Abnormal termination",                              {}, 0},
        {SIGFPE,  "SIGFPE",  "Floating-point exception",                          {}, 0},
        {SIGBUS,  "SIGBUS",  "Bus error (bad memory access)",                     {}, 0},
        {SIGSEGV, "SIGSEGV", "Segmentation violation (invalid memory reference)", {}, 0},
};

static size_t observedSignalCnt = sizeof(observedSignals) / sizeof(observedSignals[0]);

static volatile sig_atomic_t initialized = 0;
static volatile sig_atomic_t intercepting = 0;

/**
 * Intercept a raised signal
 */
void interceptor(int signo, siginfo_t *_siginfo, void *ucontext) {
    const auto *_ucontext = static_cast<const ucontext_t *>(ucontext);
    observed_signal_t *signal = observed_signal_get_or_null(signo);

    if (nullptr == signal) {
        _LOGE("Can't reference observed_signal element for signal[%d]", signo);
        return;
    }

    if (!intercepting++) {
        _LOGD("Signal %d intercepted: %s", signal->signo, signal->description);

        signal->intercepting++;
        _LOGD("Observer for signal[%d] is intercepting [%d callers]", signal->signo,
              signal->intercepting);

        if (1 == signal->intercepting) {
            char *buffer = new char[BACKTRACE_SZ_MAX];
            if (collect_backtrace(buffer, BACKTRACE_SZ_MAX, _siginfo, _ucontext)) {
                serializer::from_crash(buffer, std::strlen(buffer));
            }
            delete[] buffer;
        }

        // Uninstall the custom handler prior to calling the previous sigaction (to prevent recursion)
        // uninstall_handler();
        sigutils::uninstall_handler(signal->signo, &signal->sa_previous);

        // chain to previous signo handler for abend
        invoke_previous_sigaction(signo, _siginfo, ucontext);

        signal->intercepting--;

        intercepting--;
    } else {
        _LOGE("Interceptor for signal[%d] is already intercepting, delegating to previous sigaction",
              signo);
        // chain to previous signo handler for abend
        invoke_previous_sigaction(signo, _siginfo, ucontext);
    }
}

/**
 * Install observed signal handlers
 *
 * SA_ONSTACK: handle the signal on separate stack
 **/
void *install_signal_observers(__unused void *unused) {

    if (!initialized++) {
        // The thread name is restricted to 16 chars, including the terminating null
        if (0 != pthread_setname_np(pthread_self(), "NR-Sig-Handler")) {
            _LOGE_POSIX("pthread_setname_np()");
        } else {
            for (size_t i = 0; i < observedSignalCnt; i++) {
                const observed_signal_t *signal = &observedSignals[i];
                if (sigutils::install_handler(signal->signo, interceptor,
                                              &signal->sa_previous, SA_ONSTACK)) {
                    _LOGI("Signal %d [%s] handler installed", signal->signo, signal->description);
                } else {
                    _LOGE("Unable to install signal %d handler", signal->signo);
                }
            }

            intercepting = 0;

            _LOGI("Signal handler initialized");
        }
    }

    return nullptr;
}

/**
 * Remove all installed signal interceptors
 */
void uninstall_handler() {
    if (initialized > 0) {
        for (size_t i = 0; i < observedSignalCnt; i++) {
            observed_signal_t signal = observedSignals[i];
            // sigaction(signal.signo, &signal.sa_previous, nullptr);
            if (!sigutils::uninstall_handler(signal.signo, &signal.sa_previous)) {
                _LOGE_POSIX(signal.name);
            }
        }

        initialized--;

        _LOGI("Signal handler uninstalled");
    }
}

/**
 * release all alloc'd heap:
 */
void dealloc() {
    if (_stack.ss_sp != nullptr) {
        free(_stack.ss_sp);
        _stack.ss_sp = nullptr;
        _LOGI("Handler signal stack freed(%zu bytes)", _stack.ss_size);
    }
}

bool signal_handler_initialize() {
    if (0 == pthread_mutex_lock(&mutex)) {

        if (!sigutils::set_sigstack(&_stack, SIGSTKSZ * 2)) {
            _LOGE("Signal handlers are disabled: could not set the handler signal stack");
            return false;
        }

        // Main thread does not block SIGQUIT by default.
        // Block it and start a new thread to handle all signals,
        // using the signal mask of the parent thread.

        if (sigutils::block_signal(SIGQUIT)) {
            auto handlerThread = (pthread_t) nullptr;

            // new thread will inherit the sigmask of the parent
            if (0 != pthread_create(&handlerThread, nullptr, install_signal_observers, nullptr)) {
                _LOGE_POSIX("Unable to create monitor thread");
            }

            // restore SIGQUIT on this thread
            sigutils::unblock_signal(SIGQUIT);
        }

        if (0 != pthread_mutex_unlock(&mutex)) {
            _LOGE_POSIX("pthread_mutex_unlock()");
        }
    } else {
        _LOGE_POSIX("pthread_mutex_lock() failed");
    }

    return true;
}

/**
 * Remove all installed signal interceptors, clean up all alloc'd memory
 */
void signal_handler_shutdown() {
    _LOGI("Shutting down signal handler");
    if (0 == pthread_mutex_lock(&mutex)) {
        uninstall_handler();
        dealloc();
        if (0 == pthread_mutex_unlock(&mutex)) {
            _LOGI("The signal handler has shutdown");
            return;
        }
    }
    _LOGE_POSIX("pthread_mutex_lock() failed");
}

/**
 * Invoke the default handler for a passed signal
 */
void invoke_sigaction(int signo, struct sigaction *_sigaction,
                      siginfo_t *_siginfo, void *ucontext) {
    if (_sigaction->sa_flags & SA_SIGINFO) {
        _LOGD("Calling signal[%d] sigaction w/siginfo", signo);
        _sigaction->sa_sigaction(signo, _siginfo, ucontext);

    } else if (_sigaction->sa_handler == SIG_DFL) {
        _LOGD("Calling signal[%d] default handler", signo);
        raise(signo);

    } else if (_sigaction->sa_handler != SIG_IGN) {
        _LOGD("Signal [%d] action gnored", signo);
        void (*handler)(int) = _sigaction->sa_handler;
        handler(signo);
    }
}

/**
 * The process may be killed during this function execution and may never return
 */
void invoke_previous_sigaction(int signo, siginfo_t *_siginfo, void *ucontext) {
    if (0 == pthread_mutex_lock(&mutex)) {
        for (size_t i = 0; i < observedSignalCnt; ++i) {
            observed_signal_t signal = observedSignals[i];
            if (signal.signo == signo) {
                _LOGI("Invoking previous handler for signal %d", signal.signo);
                invoke_sigaction(signo, &signal.sa_previous, _siginfo, ucontext);
            }
        }
        if (0 != pthread_mutex_unlock(&mutex)) {
            _LOGE_POSIX("pthread_mutex_unlock() failed");
        }
    } else {
        _LOGE_POSIX("pthread_mutex_lock() failed");
    }
}

observed_signal_t *observed_signal_get_or_null(int signo) {
    for (size_t i = 0; i < observedSignalCnt; ++i) {
        if (observedSignals[i].signo == signo) {
            return &observedSignals[i];
        }
    }

    return nullptr;
}