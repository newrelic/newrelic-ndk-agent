/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_EMITTER_H
#define _AGENT_NDK_EMITTER_H

#include <agent-ndk.h>

const char *emit_backtrace(backtrace_t &, std::string &);

#endif // _AGENT_NDK_EMITTER_H

