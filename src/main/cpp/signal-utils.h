/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_SIGNAL_UTILS_H
#define _AGENT_NDK_SIGNAL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

namespace sigutils {

    bool set_sigstack(stack_t *_stack, size_t stackSize);

    bool block_signal(int signo);

    bool unblock_signal(int signo);

    bool install_handler(int signo, void sig_action(int, siginfo_t*, void*));

}   // namespace sigutils

#ifdef __cplusplus
}
#endif


#endif // _AGENT_NDK_SIGNAL_UTILS_H
