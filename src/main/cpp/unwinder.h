/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_UNWINDER_H
#define _AGENT_NDK_UNWINDER_H

#include <agent-ndk.h>

#ifdef _DEMANGLE_CXX
#define IN_LIBGCC2 1    // define __cxxabiv1::__cxa_demangle
namespace __cxxabiv1 {
    extern "C" {
        #include "demangle/cp-demangle.c"
    }
}
#endif // _DEMANGLE_CXX

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEMANGLE_CXX
char* __cxa_demangle(const char* mangled, char* demangled, size_t* buffsz, int* status);
#endif  // _DEMANGLE_CXX

bool unwind_backtrace(char*, size_t, const siginfo_t*, const ucontext_t*);

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_UNWINDER_H
