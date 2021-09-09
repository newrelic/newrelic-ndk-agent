/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include <cerrno>

#include <agent-ndk.h>
#include "signal-utils.h"

namespace sigutils {

bool set_sigstack(stack_t *_stack, size_t stackSize) {
    _LOGD("sigutils::set_sigstack (%zu) bytes to stack[%p]", stackSize, _stack);

    // caller must release this memory
    _stack->ss_sp = calloc(1, stackSize);
    if (_stack->ss_sp != nullptr) {
        _stack->ss_size = stackSize;
        _stack->ss_flags = 0;
        int rc = sigaltstack(_stack, 0);
        if (rc != 0) {
            _LOGE("Could not set the stack: %s", std::strerror(rc));
            return false;
        }
    } else {
        _LOGE("Signal handler is disabled: could not alloc %zu-byte stack", stackSize);
        return false;
    }

    return true;
}

bool block_signal(int signo) {
    _LOGD("sigutils::block_signal(%d) on thread [%d]", signo, gettid());

    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, signo);
    if (pthread_sigmask(SIG_BLOCK, &sigmask, nullptr) != 0) {
        _LOGE("Could not block signal [%d]: %s", signo, std::strerror(errno));
        return false;
    }

    return true;
}

bool unblock_signal(int signo) {
    _LOGD("sigutils::unblock_signal(%d) on thread [%d]", signo, gettid());

    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, signo);
    if (pthread_sigmask(SIG_UNBLOCK, &sigmask, nullptr) != 0) {
        _LOGE("Could not unblock signal [%d]: %s", signo, std::strerror(errno));
        return false;
    }

    return true;
}

bool install_handler(int signo, void sig_action(int, siginfo_t*, void*)) {
    _LOGD("sigutils::install_handler(%d, %p) on thread [%d]", signo, sig_action, gettid());

    struct sigaction handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_sigaction = sig_action;
    handler.sa_flags = SA_SIGINFO;
    if (sigaction(signo, &handler, NULL) != 0) {
        _LOGE("Could not install signal[%d] handler: %s.", signo, strerror(errno));
        return false;
    }

    return true;
}

}   // namespace sigutils
