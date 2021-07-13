/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <signal.h>
#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>

#include "signal-handlers.h"

typedef struct observed_signal {
    int signo;
    const char *name;
    const char *description;
    struct sigaction sa_previous;

} observed_signal_t;

void invoke_sigaction(int signo, struct sigaction *_sigaction, siginfo_t *_siginfo, void *context);

void invoke_previous(int signo, siginfo_t *_siginfo, void *context);

void uninstall_handlers();

static const char *TAG = "[newrelic]";

// Limit backtrace to 64k
static const int BACKTRACE_MAX = 0x10000;

/* signal mask */
static sigset_t _sigset;

/* replacement stack */
static stack_t _stack;

/* the replacement signal handler */
static struct sigaction signalHandler;

/* Module-wide mutex */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Collection of observed signals */
static observed_signal_t observedSignals[] = {
        {SIGQUIT, "SIGQUIT", "Application not responding (ANR)"},
        {SIGABRT, "SIGABRT", "Abnormal termination"},
        {SIGSEGV, "SIGSEGV", "Segmentation violation (invalid memory reference)"},
        {SIGILL,  "SIGILL",  "Illegal instruction"},
        {SIGBUS,  "SIGBUS",  "Bus error (bad memory access)"},
        {SIGFPE,  "SIGFPE",  "Floating-point exception"},
};

static const size_t observedSignalCnt = sizeof(observedSignals) / sizeof(observedSignals[0]);

static bool initialized = false;


/**
 * Intercept a raised signal
 */
void interceptor(int signo, siginfo_t *_siginfo, void *ucontext) {
    for (int i = 0; i < observedSignalCnt; i++) {
        if (observedSignals[i].signo == signo) {
            const observed_signal_t *signal = &observedSignals[i];

            __android_log_print(ANDROID_LOG_INFO, TAG,
                                "Signal %d raised: %s", signal->signo, signal->description);

            switch (signo) {
                case SIGQUIT:
                    __android_log_print(ANDROID_LOG_INFO, TAG, "ANR detected");
                    // TODO handle ANR
                    break;

                case SIGKILL:
                case SIGILL:
                case SIGBUS:
                case SIGABRT:
                case SIGFPE:
                    // TODO handle this signal
                    break;

                default:
                    __android_log_print(ANDROID_LOG_ERROR, TAG, "Unsupported signal %d detected!",
                                        signo);
                    break;
            }

            break;
        }
    }

    // Uninstall the custom handlers to prevent recursion
    uninstall_handlers();

    // chain to previous signo handler for abend
    invoke_previous(signo, _siginfo, ucontext);
}

void *install_handlers() {
    if (!initialized) {
        for (int i = 0; i < observedSignalCnt; i++) {
            const observed_signal_t *signal = &observedSignals[i];
            if (0 != sigaction(signal->signo, &signalHandler,
                               (struct sigaction *) &signal->sa_previous)) {
                __android_log_print(ANDROID_LOG_ERROR, TAG,
                                    "Unable to install interceptor signal: %d", signal->signo);
            } else {
                __android_log_print(ANDROID_LOG_INFO, TAG,
                                    "Signal %d interceptor installed", signal->signo);

            }
        }

        initialized = true;
        __android_log_print(ANDROID_LOG_INFO, TAG, "Signal handlers initialized");
    }

    return NULL;
}

/**
 * Remove all installed signal interceptors
 */
void uninstall_handlers() {
    if (initialized) {
        for (int i = 0; i < observedSignalCnt; i++) {
            observed_signal_t signal = observedSignals[i];
            sigaction(signal.signo, &signal.sa_previous, 0);
            memset(&signal.sa_previous, 0, sizeof(struct sigaction));
            __android_log_print(ANDROID_LOG_INFO, TAG, "Signal handler (%d) uninstalled",
                                signal.signo);
        }

        signalHandler.sa_handler = SIG_DFL;         // the default interceptor
        signalHandler.sa_flags = SA_RESETHAND;      // default signal flags

        initialized = false;
        __android_log_print(ANDROID_LOG_INFO, TAG, "Signal handlers uninstalled");
    }
}

/**
 * release all alloc'd heap:
 */
void dealloc() {
    if (_stack.ss_sp != NULL) {
        free(_stack.ss_sp);
        _stack.ss_sp = NULL;
        __android_log_print(ANDROID_LOG_INFO, TAG, "Handler stack memory freed(%zu bytes)",
                            _stack.ss_size);
    }
}

bool signal_handler_initialize() {
    pthread_t handlerThread = (pthread_t) NULL;

    pthread_mutex_lock(&mutex);

    sigemptyset(&_sigset);

    size_t stackSize = SIGSTKSZ * 2;
    _stack.ss_sp = calloc(1, stackSize);
    if (_stack.ss_sp != NULL) {
        _stack.ss_size = stackSize;
        _stack.ss_flags = 0;
        if (sigaltstack(&_stack, 0) < 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Could not set the stack");
            return false;
        } else {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Handler stack created %zu-byte stack",
                                stackSize);
        }
    } else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Could not alloc %zu-byte stack", stackSize);
        return false;
    }

    // the signal handler
    memset(&signalHandler, 0, sizeof(struct sigaction));
    sigemptyset(&signalHandler.sa_mask);                // clear the action mask
    signalHandler.sa_sigaction = interceptor;               // set the action intercept handler
    signalHandler.sa_flags = (SA_SIGINFO | SA_ONSTACK);     // set the action signal flags

    // Main thread does not block SIGQUIT by default.
    // Block it and start a new thread to handle all signals,
    // using the signal mask of the parent thread.

    sigemptyset(&_sigset);
    sigaddset(&_sigset, SIGQUIT);

    if (pthread_sigmask(SIG_BLOCK, &_sigset, NULL) == 0) {
        if (pthread_create(&handlerThread, NULL, install_handlers, NULL) != 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Unable to create watchdog thread");
        }

        // reset defaults on the main thread
        if (pthread_sigmask(SIG_UNBLOCK, &_sigset, NULL) != 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Unable to restore mask on SIGQUIT");
        }

    } else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Unable to mask SIGQUIT");
    }

    pthread_mutex_unlock(&mutex);

    return true;
}

/**
 * Remove all installed signal interceptors, clean up all alloc'd memory
 */
void signal_handler_shutdown() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "Shutting down signal handler");
    pthread_mutex_lock(&mutex);
    uninstall_handlers();
    dealloc();
    pthread_mutex_unlock(&mutex);
    __android_log_print(ANDROID_LOG_INFO, TAG, "The signal handler has shutdown");
}

/**
 * Invoke the proper handler for a passed signal
 */
void invoke_sigaction(int signo, struct sigaction *_sigaction, siginfo_t *_siginfo, void *ucontext) {

    if (_sigaction->sa_flags & SA_SIGINFO) {
        __android_log_print(ANDROID_LOG_INFO, TAG,
                            "Signal %d rcvd: calling sigaction w/siginfo", signo);
        _sigaction->sa_sigaction(signo, _siginfo, ucontext);

    } else if (_sigaction->sa_handler == SIG_DFL) {
        __android_log_print(ANDROID_LOG_INFO, TAG,
                            "Signal %d rcvd: calling default handler", signo);
        raise(signo);

    } else if (_sigaction->sa_handler != SIG_IGN) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "Signal %d rcvd: ignored", signo);
        void (*handler)(int) = _sigaction->sa_handler;
        handler(signo);
    }
}

/**
 * The process may be killed during this function execution and may never return
 */
void invoke_previous(int signo, siginfo_t *_siginfo, void *ucontext) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < observedSignalCnt; ++i) {
        observed_signal_t signal = observedSignals[i];
        if (signal.signo == signo) {
            __android_log_print(ANDROID_LOG_INFO, TAG,
                                "Invoking previous handler for signal %d", signal.signo);
            invoke_sigaction(signo, &signal.sa_previous, _siginfo, ucontext);
        }
    }

    pthread_mutex_unlock(&mutex);
}

