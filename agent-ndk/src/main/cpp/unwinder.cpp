/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 **/

#include <unwind.h>
#include <dlfcn.h>
#include <sys/ucontext.h>
#include <asm/sigcontext.h>
#include <cxxabi.h>

#include <agent-ndk.h>
#include "backtrace.h"
#include "unwinder.h"
#include "procfs.h"

/**
 * Get the crash ip, given a pointer to a ucontext_t context.
 **/
static uintptr_t crash_ip_from_mcontext(const mcontext_t *mcontext) {

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
        _LOGE("record_frame[%zu]: stack is full", state->frame_cnt);
        return false;
    }

#if __thumb__
    // reset the Thumb bit if set
    const uintptr_t thumb_bit = 1;
    ip &= ~thumb_bit;
#endif  // __thumb__

    if (state->frame_cnt > 0) {
        // ignore null frames
        if (ip == (uintptr_t) nullptr ) {
            _LOGW("record_frame[%zu]: Ignoring null ip", state->frame_cnt);
            state->skipped_frames++;
            return true;
        }

        // ignore duplicate frames
        if (ip == state->frames[state->frame_cnt - 1]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"
            _LOGW("record_frame[%zu]: ip[%zu] duplicate of frame[%lu]: ip[%zu]",
                  state->frame_cnt, ip, state->frame_cnt - 1, state->frames[state->frame_cnt - 1]);
#pragma clang diagnostic pop
            state->skipped_frames++;
            return true;
        }
    }

    // Finally add the address to the storage
    _LOGI("record_frame[%zu]: adding address[%zu]", state->frame_cnt, ip);
    state->frames[state->frame_cnt++] = ip;

    return true;
}

void transform_addr_to_stackframe(size_t index, uintptr_t address, stackframe_t &stackframe) {
    Dl_info info = {};

    stackframe.index = index;
    stackframe.address = address;

    _LOGE("Resolving frame[%zu]: addr[%zu]", index, address);
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

        // Relative addresses appear when code is compiled with
        // _position-independent code_ (-fPIC, -pie) options.
        // Android requires position-independent code since Android 5.0.
        stackframe.so_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
        stackframe.sym_addr = reinterpret_cast<uintptr_t>(info.dli_saddr);
        if (stackframe.sym_addr != 0) {
            stackframe.sym_addr_offset = reinterpret_cast<uintptr_t>(address - stackframe.sym_addr);
        }

        stackframe.pc = address - stackframe.so_base;
    }
}

_Unwind_Reason_Code unwinder_cb(struct _Unwind_Context *ucontext, void *arg) {
    backtrace_state_t *state = static_cast<backtrace_state_t *>(arg);
    int ip_before = 0;

    _Unwind_Ptr ip = _Unwind_GetIPInfo(ucontext, &ip_before);

    if (ip_before != 0) {
        // IP is before or after first not yet fully executed instruction
        _LOGI("unwinder_cb[%zu]: ip_before[%d]: address[%zu]", state->frame_cnt, ip_before, ip);
    }

    if (ip == state->crash_ip) {
        _LOGI("unwinder_cb[%zu]: crash address[%zu]", state->frame_cnt, ip);
        state->skipped_frames = state->frame_cnt;
        state->frame_cnt = 0;   // reset the index
    } else if (ip > 0) {
#if defined(__aarch64__)
        // TODO find out why we need this adjustment to match Android offset values
        ip -= sizeof(u_int32_t);
#endif  // __aarch64__
    }

    return record_frame(ip, state) ? _URC_NO_REASON : _URC_END_OF_STACK;
}

bool unwind_backtrace(backtrace_state_t &state) {

    if (state.sa_ucontext == nullptr) {
        _LOGE("unwind_backtrace: sa_ucontext is null");
        return 0;
    }

    // get pointer to machine specific context
    const mcontext_t *mcontext = &(state.sa_ucontext->uc_mcontext);
    if (mcontext == nullptr) {
        _LOGE("unwind_backtrace: uc_mcontext is null");
        return 0;
    }

    state.skipped_frames = 0;
    state.frame_cnt = 0;
    state.crash_ip = crash_ip_from_mcontext(mcontext);

    // unwinds the backtrace and fills the buffer with stack frame addresses
    _Unwind_Backtrace(unwinder_cb, &state);

    _LOGD("[%s] unwind_backtrace: frames[%zu] skipped[%d] context[%p]",
          get_arch(), state.frame_cnt, state.skipped_frames, state.sa_ucontext);

    return true;
}
