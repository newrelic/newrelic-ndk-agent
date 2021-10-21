/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include <cerrno>
#include <unistd.h>

#include <agent-ndk.h>
#include "signal-utils.h"

#ifndef SI_TKILL
#define SI_TKILL -6
#endif

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

    bool install_handler(int signo,
                         void sig_action(int, siginfo_t *, void *),
                         const struct sigaction *current_sig_action, int sa_flags) {
        _LOGD("sigutils::install_handler(%d, %p, %p) on thread [%d]",
              signo, sig_action, current_sig_action, gettid());

        struct sigaction handler = {};
        sigemptyset(&handler.sa_mask);
        handler.sa_flags = SA_SIGINFO | sa_flags;
        handler.sa_sigaction = sig_action;

        if (sigaction(signo, &handler, const_cast<struct sigaction *>(current_sig_action)) != 0) {
            _LOGD("Could not install signal[%d] handler: %s.", signo, strerror(errno));
            return false;
        }

        return true;
    }

    const char *get_subcode_description(int code, const char *default_description) {
        switch (code) {
            case SI_TKILL:
                return "SIG_TKILL";
        };
        return default_description;
    }

    /**
     * Signal descriptions: <http://pubs.opengroup.org/onlinepubs/009696699/basedefs/signal.h.html>
     * @param signo The integer signal number
     * @param code Subcode of the signal. If -1, return simple literal of the signal number
     */
    const char *get_signal_description(int signo, int code) {
        switch (signo) {
            case SIGILL:
                switch (code) {
                    case -1:
                        return "SIGILL";
                    case ILL_ILLOPC:
                        return "Illegal opcode";
                    case ILL_ILLOPN:
                        return "Illegal operand";
                    case ILL_ILLADR:
                        return "Illegal addressing mode";
                    case ILL_ILLTRP:
                        return "Illegal trap";
                    case ILL_PRVOPC:
                        return "Privileged opcode";
                    case ILL_PRVREG:
                        return "Privileged register";
                    case ILL_COPROC:
                        return "Coprocessor error";
                    case ILL_BADSTK:
                        return "Internal stack error";
                    default:
                        return get_subcode_description(code, "Illegal operation");
                }
                break;
            case SIGTRAP:
                switch (code) {
                    case -1:
                        return "SIGTRAP";
                    case TRAP_BRKPT:
                        return "Process breakpoint";
                    case TRAP_TRACE:
                        return "Process trace trap";
                    default:
                        return get_subcode_description(code, "Trap");
                }
                break;
            case SIGABRT:
                switch (code) {
                    case -1:
                        return "SIGABRT";
                    default:
                        return get_subcode_description(code, "Process abort signal");
                }
            case SIGSEGV:
                switch (code) {
                    case -1:
                        return "SIGSEGV";
                    case SEGV_MAPERR:
                        return "Address not mapped to object";
                    case SEGV_ACCERR:
                        return "Invalid permissions for mapped object";
                    default:
                        return get_subcode_description(code, "Segmentation violation");
                }
                break;
            case SIGFPE:
                switch (code) {
                    case -1:
                        return "SIGFPE";
                    case FPE_INTDIV:
                        return "Integer divide by zero";
                    case FPE_INTOVF:
                        return "Integer overflow";
                    case FPE_FLTDIV:
                        return "Floating-point divide by zero";
                    case FPE_FLTOVF:
                        return "Floating-point overflow";
                    case FPE_FLTUND:
                        return "Floating-point underflow";
                    case FPE_FLTRES:
                        return "Floating-point inexact result";
                    case FPE_FLTINV:
                        return "Invalid floating-point operation";
                    case FPE_FLTSUB:
                        return "Subscript out of range";
                    default:
                        return get_subcode_description(code, "Floating-point");
                }
                break;
            case SIGBUS:
                switch (code) {
                    case -1:
                        return "SIGBUS";
                    case BUS_ADRALN:
                        return "Invalid address alignment";
                    case BUS_ADRERR:
                        return "Nonexistent physical address";
                    case BUS_OBJERR:
                        return "Object-specific hardware error";
                    default:
                        return get_subcode_description(code, "Bus error");
                }
                break;
            case SIGINT:
                switch (code) {
                    case -1:
                        return "SIGINT";
                    default:
                        return get_subcode_description(code, "Terminal interrupt signal");
                }
                break;
            case SIGKILL:
                switch (code) {
                    case -1:
                        return "SIGKILL";
                    default:
                        return get_subcode_description(code, "Killed");
                }
                break;
            case SIGQUIT:
                switch (code) {
                    case -1:
                        return "SIGQUIT";
                    default:
                        return get_subcode_description(code, "Terminal quit signal (ANR)");
                }
                break;
            default:
                return "UNKNOWN";
                break;
        }
    }

}   // namespace sigutils
