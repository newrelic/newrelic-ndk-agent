/**
 * Copyright 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_BACKTRACE_H
#define _AGENT_NDK_BACKTRACE_H

#include <agent-ndk.h>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif  // !PATH_MAX

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

#ifndef NGREG
#if defined(__arm__)
#define NGREG 16
#elif defined(__aarch64__)
#define NGREG 34
#elif defined(__i386__)
#define NGREG 16
#elif defined(__x86_64__)
#define NGREG 34
#endif  // defined(__arm__)
#endif  // !NGREG


/**
 * Unwinder backtrace metadata
 */

typedef struct backtrace_state {
    uintptr_t frames[BACKTRACE_FRAMES_MAX];
    size_t frame_cnt;
    int skipped_frames;
    uintptr_t crash_ip;
    const ucontext_t *sa_ucontext;
    const siginfo_t *siginfo;

}   backtrace_state_t;


/**
 * A stackframe represents the resolved address data for a single calling frame
 */
typedef struct stackframe {
    size_t index;               // 0-based index of this frame in stack (top down)
    uintptr_t address;          // Instruction pointer value (address)
    uintptr_t pc;               // Program counter
    char so_path[PATH_MAX];     // Pathname of shared object containing ip
    char sym_name[255];         // Name of symbol whose definition overlaps ip
    uintptr_t so_base;          // Base address of shared object
    uintptr_t sym_addr;         // Address of nearest symbol
    uintptr_t sym_addr_offset;  // Offset from symbol

}   stackframe_t;

/**
 * Thread state metadata
 */
typedef struct threadinfo {
    int tid;                    // Thread ID
    char thread_name[32];       // Name of thread, if set (as reported in /procfs)
    bool crashed;               // True if this thread caused the violation
    char thread_state[16];      // State of thread (as reported in /procfs)
    int priority;               // Priority of thread (as reported in /procfs)
    uintptr_t stack;            // Stack address (base)

    backtrace_state_t*  backtrace_state;

}   threadinfo_t;


/**
 * A backtrace represents the state of the machine at the point of violation:
 */
typedef struct backtrace {
    backtrace_state_t state;

    char description[128];
    long timestamp;
    char arch[16];
    int pid;
    int ppid;
    int uid;

    std::vector<threadinfo_t> threads;

}   backtrace_t;


#endif // _AGENT_NDK_BACKTRACE_H
