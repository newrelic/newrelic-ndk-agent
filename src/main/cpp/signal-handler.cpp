/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>
#include <agent-ndk.h>

#include "signal-utils.h"
#include "signal-handler.h"
#include "unwinder.h"

typedef struct observed_signal {
    int signo;
    const char *name;
    const char *description;
    struct sigaction sa_previous;

} observed_signal_t;

/* forward decls */
void invoke_sigaction(int signo, struct sigaction *_sigaction, siginfo_t *_siginfo, void *context);

void invoke_previous_sigaction(int signo, siginfo_t *_siginfo, void *context);

/* our handler interceptors */
void install_handler();

void uninstall_handler();

/* report serialization methods */
void emit_backtrace(const char *);


/* signal mask */
static sigset_t _sigset;

/* replacement stack */
static stack_t _stack = {};

/* the replacement signal handler */
static struct sigaction signalHandler = {};

/* Module-wide mutex */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Collection of observed signals */
static observed_signal_t observedSignals[] = {
        {SIGILL,  "SIGILL",  "Illegal instruction",                               {}},
        {SIGTRAP, "SIGTRAP", "Breakpoint",                                        {}},
        {SIGABRT, "SIGABRT", "Abnormal termination",                              {}},
        {SIGFPE,  "SIGFPE",  "Floating-point exception",                          {}},
        {SIGBUS,  "SIGBUS",  "Bus error (bad memory access)",                     {}},
        {SIGSEGV, "SIGSEGV", "Segmentation violation (invalid memory reference)", {}},
};

static size_t observedSignalCnt = sizeof(observedSignals) / sizeof(observedSignals[0]);

static bool initialized = false;


/**
 * Intercept a raised signal
 */
void interceptor(int signo, siginfo_t *_siginfo, void *ucontext) {
    const ucontext_t *_ucontext = static_cast<const ucontext_t *>(ucontext);
    char buffer[BACKTRACE_SZ_MAX];

    // Uninstall the custom handler to prevent recursion
    uninstall_handler();

    for (size_t i = 0; i < observedSignalCnt; i++) {
        if (observedSignals[i].signo == signo) {
            const observed_signal_t *signal = &observedSignals[i];

            _LOGD("Signal %d raised: %s", signal->signo, signal->description);

            switch (signo) {
                case SIGQUIT:
                    // ANRs are managed in anr-handler.cpp
                    // collectBacktrace(buffer, sizeof(buffer), _siginfo, _ucontext);
                    // emit_backtrace(buffer);
                    break;

                case SIGILL:
                case SIGTRAP:
                case SIGABRT:
                case SIGFPE:
                case SIGBUS:
                case SIGSEGV:
                    collectBacktrace(buffer, sizeof(buffer), _siginfo, _ucontext);
                    emit_backtrace(buffer);
                    _LOGI("Signal raised: %s", buffer);
                    break;

                default:
                    _LOGE("Unsupported signal %d detected!", signo);
                    break;
            }

            break;
        }
    }

    // chain to previous signo handler for abend
    invoke_previous_sigaction(signo, _siginfo, ucontext);
}

/**
 * Install observed signals
 */
void *install_handler(__unused void *unused) {

    if (!initialized) {
        for (size_t i = 0; i < observedSignalCnt; i++) {
            const observed_signal_t *signal = &observedSignals[i];
            if (0 != sigaction(signal->signo, &signalHandler,
                               (struct sigaction *) &signal->sa_previous)) {
                _LOGE("Unable to install signal %d handler", signal->signo);
            } else {
                _LOGI("Signal %d [%s] handler installed", signal->signo, signal->description);
            }
        }

        initialized = true;
        _LOGI("Signal handler initialized");
    }

    return nullptr;
}

/**
 * Remove all installed signal interceptors
 */
void uninstall_handler() {

    if (initialized) {
        for (size_t i = 0; i < observedSignalCnt; i++) {
            observed_signal_t signal = observedSignals[i];
            sigaction(signal.signo, &signal.sa_previous, nullptr);
            memset(&signal.sa_previous, 0, sizeof(struct sigaction));
            // _LOGD("Signal %d interceptor uninstalled", signal.signo);
        }

        signalHandler.sa_handler = SIG_DFL;         // the default handler
        signalHandler.sa_flags = SA_RESETHAND;      // default signal flags

        initialized = false;
        _LOGI("Signal handler uninstalled");
    }
}

/**
 * release all alloc'd heap:
 */
void dealloc() {

    if (_stack.ss_sp != NULL) {
        free(_stack.ss_sp);
        _stack.ss_sp = NULL;
        _LOGI("Handler stack memory freed(%zu bytes)", _stack.ss_size);
    }
}

bool signal_handler_initialize() {
    pthread_t handlerThread = (pthread_t) NULL;

    pthread_mutex_lock(&mutex);

    if (!sigutils::set_sigstack(&_stack, SIGSTKSZ * 2)) {
        _LOGE("Signal handlers are disabled: could not set the stack");
        return false;
    }

    // the replacement signal handler
    memset(&signalHandler, 0, sizeof(struct sigaction));
    sigemptyset(&signalHandler.sa_mask);                // clear the action mask
    signalHandler.sa_sigaction = interceptor;               // set the action intercept handler
    signalHandler.sa_flags = (SA_SIGINFO | SA_ONSTACK);     // set the action signal flags

    // Main thread does not block SIGQUIT by default.
    // Block it and start a new thread to handle all signals,
    // using the signal mask of the parent thread.

    sigemptyset(&_sigset);
    sigaddset(&_sigset, SIGQUIT);

    if (pthread_sigmask(SIG_BLOCK, &_sigset, nullptr) == 0) {
        if (pthread_create(&handlerThread, nullptr, install_handler, nullptr) != 0) {
            _LOGE("Unable to create watchdog thread");
        }

        // reset defaults on the main thread
        if (pthread_sigmask(SIG_UNBLOCK, &_sigset, NULL) != 0) {
            _LOGE("Unable to restore mask on SIGQUIT");
        }

    } else {
        _LOGE("Unable to mask SIGQUIT");
    }

    pthread_mutex_unlock(&mutex);

    return true;
}

/**
 * Remove all installed signal interceptors, clean up all alloc'd memory
 */
void signal_handler_shutdown() {
    _LOGI("Shutting down signal handler");
    pthread_mutex_lock(&mutex);
    uninstall_handler();
    dealloc();
    pthread_mutex_unlock(&mutex);
    _LOGI("The signal handler has shutdown");
}

/**
 * Invoke the proper handler for a passed signal
 */
void
invoke_sigaction(int signo, struct sigaction *_sigaction, siginfo_t *_siginfo, void *ucontext) {

    if (_sigaction->sa_flags & SA_SIGINFO) {
        _LOGI("Signal %d rcvd: calling sigaction w/siginfo", signo);
        _sigaction->sa_sigaction(signo, _siginfo, ucontext);

    } else if (_sigaction->sa_handler == SIG_DFL) {
        _LOGI("Signal %d rcvd: calling default handler", signo);
        raise(signo);

    } else if (_sigaction->sa_handler != SIG_IGN) {
        _LOGI("Signal %d rcvd: ignored", signo);
        void (*handler)(int) = _sigaction->sa_handler;
        handler(signo);
    }
}

/**
 * The process may be killed during this function execution and may never return
 */
void invoke_previous_sigaction(int signo, siginfo_t *_siginfo, void *ucontext) {
    pthread_mutex_lock(&mutex);

    for (size_t i = 0; i < observedSignalCnt; ++i) {
        observed_signal_t signal = observedSignals[i];
        if (signal.signo == signo) {
            _LOGD("Invoking previous handler for signal %d", signal.signo);
            invoke_sigaction(signo, &signal.sa_previous, _siginfo, ucontext);
        }
    }

    pthread_mutex_unlock(&mutex);
}

/**
 * Serialize the backtrace to local storage, to be picked up for processing on
 * the next app invocation
 */
void emit_backtrace(const char *buffer) {
    // TODO
}
