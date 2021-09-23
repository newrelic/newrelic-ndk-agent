/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_UNWINDER_H
#define _AGENT_NDK_UNWINDER_H

#include <agent-ndk.h>

#ifdef __cplusplus
extern "C" {
#endif

bool unwind_backtrace(char *, size_t, const siginfo_t *, const ucontext_t *);

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_UNWINDER_H
