/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_TERMINATE_HANDLER_H
#define _AGENT_NDK_TERMINATE_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

bool terminate_handler_initialize();

void terminate_handler_shutdown();

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_TERMINATE_HANDLER_H
