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

bool unwind_backtrace(backtrace_state_t &);

#ifdef __cplusplus
}
#endif

void transform_addr_to_stackframe(size_t, uintptr_t, stackframe_t &);


#endif // _AGENT_NDK_UNWINDER_H
