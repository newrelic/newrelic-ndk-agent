/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_BACKTRACE_H
#define _AGENT_NDK_BACKTRACE_H

#include <agent-ndk.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif   // PATH_MAX

#define CLASS_NAME_MAX  255
#define METHOD_NAME_MAX 255

#ifndef THREAD_MAX
#define THREAD_MAX BACKTRACE_THREADS_MAX
#endif   // _THREAD_MAX

/**
 * Supported ABIs: https://developer.android.com/ndk/guides/abis
 *
 * x86: https://en.wikipedia.org/wiki/X86
 *
 * x86_64: https://en.wikipedia.org/wiki/X86-64
 *
 * armvabi-v7a: https://en.wikipedia.org/wiki/ARM_architecture#AArch32
 *
 * arm64_v8a: https://en.wikipedia.org/wiki/AArch64
 * Each of 31 registers can be used as a 64-bit X register (X0..X30)
 * or as a 32-bit W register (W0..W30).
 *
 **/

typedef struct stackframe {
    const char filename[PATH_MAX];
    const char classname[CLASS_NAME_MAX];
    const char methodname[METHOD_NAME_MAX];
    const int linenumber;

}   stackframe_t;

typedef struct threadinfo {
    const bool crashing_thread;
    const char thread_state[32];
    const int priority;
    const int tid;
    const stackframe_t stackframes[BACKTRACE_FRAMES_MAX];

}   threadinfo_t;

/**
 * A backtrace represents the state of the machine at crash:
 */
typedef struct backtrace {
    const char arch[16];
    long timestamp;
    const char description[128];
    int pid;
    int tid;
    int uid;
    threadinfo_t threads[THREAD_MAX];
    uint64_t registers[32];

}   backtrace_t;

#endif // _AGENT_NDK_BACKTRACE_H
