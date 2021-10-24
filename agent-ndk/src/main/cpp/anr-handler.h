/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_ANR_HANDLER_H
#define _AGENT_NDK_ANR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

bool anr_handler_initialize();

void anr_handler_shutdown();

void raise_anr_signal();

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_ANR_HANDLER_H
