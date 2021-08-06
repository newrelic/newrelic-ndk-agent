/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_ANRHANDLER_H
#define _AGENT_NDK_ANRHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

bool anr_handler_initialize();
bool anr_handler_shutdown();

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_ANRHANDLER_H
