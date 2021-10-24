/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 **/

#include <unwind.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <sys/ucontext.h>
#include <asm/sigcontext.h>
#include <sys/resource.h>
#include <cxxabi.h>
#include <sstream>

#include <agent-ndk.h>
#include "backtrace.h"
#include "unwinder.h"
#include "procfs.h"
#include "signal-utils.h"

/**
 * Get the program counter, given a pointer to a ucontext_t context.
 **/
static uintptr_t pc_from_mcontext(const mcontext_t *mcontext) {

#if defined(__i386)
    return mcontext->gregs[REG_EIP];
#elif defined(__x86_64__)
    return mcontext->gregs[REG_RIP];
#elif defined(__arm__)
    return mcontext->arm_pc;
#elif defined(__aarch64__)
    return mcontext->pc;
#else
#error "Unknown ABI"
#endif
}

static bool record_frame(uintptr_t ip, backtrace_state_t *state) {

    if (state->frame_cnt >= BACKTRACE_FRAMES_MAX) {
        _LOGE("record_frame: stack is full");
        return false;
    }

#if __thumb__
    // reset the Thumb bit if set
    const uintptr_t thumb_bit = 1;
    ip &= ~thumb_bit;
#endif  // __thumb__

    if (state->frame_cnt > 0) {
        // ignore null or duplicate frames
        if (ip == (uintptr_t) nullptr || ip == state->frames[state->frame_cnt - 1]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"
            _LOGE("record_frame: ip is null or duplicate of frame[%lu]", state->frame_cnt - 1);
#pragma clang diagnostic pop
            return true;
        }
    }

    // Finally add the address to the storage.
    state->frames[state->frame_cnt++] = ip;

    return true;
}

static void record_crashing_thread(backtrace_state_t *state) {

    if (state->sa_ucontext == nullptr) {
        _LOGE("record_crashing_thread: sa_ucontext is null");
        return;
    }

    // get pointer to machine specific context
    const mcontext_t *mcontext = &(state->sa_ucontext->uc_mcontext);
    if (mcontext == nullptr) {
        _LOGE("record_crashing_thread: uc_mcontext is null");
        return;
    }

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    record_frame(pc_from_mcontext(mcontext), state);
}

void transform_addr_to_stackframe(size_t index, uintptr_t address, stackframe_t &stackframe) {
    Dl_info info = {};

    stackframe.index = index;
    stackframe.ip = address;

    if (dladdr(reinterpret_cast<void *>(address), &info)) {

        if (info.dli_fname) {
            std::strncpy(stackframe.so_path, info.dli_fname, sizeof(stackframe.so_path));
        }

        if (info.dli_sname) {
            const char *symbol = info.dli_sname;
            int status = 0;
            const char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);
            if (demangled != nullptr && status == 0) {
                symbol = demangled;
            }
            std::strncpy(stackframe.sym_name, symbol, sizeof(stackframe.sym_name));
        }

        if (info.dli_fbase) {
            // Relative addresses appear when code is compiled with
            // _position-independent code_ (-fPIC, -pie) options.
            // Android requires position-independent code since Android 5.0.
            stackframe.so_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
            stackframe.sym_addr = reinterpret_cast<uintptr_t>(info.dli_saddr);
            stackframe.sym_addr_offset = reinterpret_cast<uintptr_t>(stackframe.sym_addr -
                                                                     stackframe.so_base);
        }
    }
}

_Unwind_Reason_Code unwinder_cb(struct _Unwind_Context *ucontext, void *arg) {
    backtrace_state_t *state = static_cast<backtrace_state_t *>(arg);

    if (state->frame_cnt == 0) {
        record_crashing_thread(state);
        return _URC_NO_REASON;
    }

    // Skip any frames that belong to the signal handler frame.
    if (state->skip_frames > 0) {
        state->skip_frames--;
        return _URC_NO_REASON;
    }

    // Sets ip_before_insn flag indicating whether that IP is before or
    // after first not yet fully executed instruction.
    int ip_before = 0;
    _Unwind_Ptr ip = _Unwind_GetIPInfo(ucontext, &ip_before);
    if (ip_before != 0) {
        if (ip > 0) {
            ip--;
        }
    }

    return record_frame(ip, state) ? _URC_NO_REASON : _URC_END_OF_STACK;
}

bool unwind_backtrace(backtrace_state_t &state) {

    state.skip_frames = 0;
    state.frame_cnt = 0;

    // unwinds the backtrace and fills the buffer with stack frame addresses
    _Unwind_Backtrace(unwinder_cb, &state);

    _LOGD("[%s] unwind_backtrace: frames[%zu] skipped[%d] context[%p]",
          get_arch(), state.frame_cnt, state.skip_frames, state.sa_ucontext);

    return true;
}
