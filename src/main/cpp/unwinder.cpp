/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 **/

#define _DEMANGLE_CXX 1

#include <unwind.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/ucontext.h>
#include <asm/sigcontext.h>

#include <agent-ndk.h>
#include "unwinder.h"
#include "procfs.h"

typedef struct backtrace {
    uintptr_t frames[BACKTRACE_FRAMES_MAX];
    size_t frame_cnt;
    int skip_frames;
    ucontext_t *sa_ucontext;
    const siginfo_t *siginfo;
    std::string cbuffer;

} backtrace_t;

// forward decls
void collect_crashing_thread(struct _Unwind_Context *, backtrace_t *);

void collect_noncrashing_threads(struct _Unwind_Context *, backtrace_t *);

bool record_frame(uintptr_t, backtrace_t *);

void transform_frame(size_t, const _Unwind_Ptr, backtrace_t *);

void _EMIT(backtrace_t *state, const char *fmt...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    state->cbuffer.append(buffer);
}

_Unwind_Reason_Code unwinder_cb(struct _Unwind_Context *ucontext, void *arg) {
    backtrace_t *state = static_cast<backtrace_t *>(arg);

    if (state->frame_cnt == 0) {
        collect_crashing_thread(ucontext, state);
    }

    // Skip some frames that belong to the signal handler frame.
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

bool record_frame(uintptr_t ip, backtrace_t *state) {

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
        if (ip == (uintptr_t) NULL || ip == state->frames[state->frame_cnt - 1]) {
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

void transform_frame(size_t index, const _Unwind_Ptr address, backtrace_t *state) {
    Dl_info info = {};
    std::string frame;

    if (dladdr(reinterpret_cast<void *>(address), &info)) {
        char buffer[0x100];

        std::snprintf(buffer, sizeof(buffer), "native: #%02zu pc", index);
        frame.append(buffer);

        std::snprintf(buffer, sizeof(buffer), " 0x%zu", address);
        frame.append(buffer);

        if (info.dli_fname) {
            frame.append(" ");
            frame.append(info.dli_fname);
        }

        if (info.dli_sname) {
            const char *symbol = info.dli_sname;

#ifdef  _DEMANGLE_CXX
            int status = 0;
            const char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);
            if (demangled != nullptr && status == 0) {
                symbol = demangled;
            }
#endif  // _DEMANGLE_CXX

            frame.append(" ");
            frame.append(symbol);
        }

        if (info.dli_fbase) {
            // Relative addresses appear when code is compiled with
            // _position-independent code_ (-fPIC, -pie) options.
            // Android requires position-independent code since Android 5.0.

            _Unwind_Ptr load_addr = reinterpret_cast<_Unwind_Ptr>(info.dli_fbase);
            _Unwind_Ptr relative_addr = reinterpret_cast<_Unwind_Ptr>(address - load_addr);

            std::snprintf(buffer, sizeof(buffer), " + 0x%zu (0x%zu)", load_addr, relative_addr);
            frame.append(buffer);
        }
    }
    _EMIT(state, "'%s'", frame.c_str());
}

/***
 * Register context
 *
 */
void collect_context(struct _Unwind_Context *ucontext, backtrace_t *state) {
    (void) ucontext;
    (void) state;

    // TODO
}

void collect_crashing_thread(__unused struct _Unwind_Context *ucontext, backtrace_t *state) {
    std::string frame;
    std::string cstr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

    _EMIT(state, "'ABI':'%s','pid':%d,'ppid':%d,'tid':%d,'uid':%d,",
          get_arch(), getpid(), getppid(), gettid(), getuid());
    _EMIT(state, "'timestamp':%ld,", time(0L));
    _EMIT(state, "'name':'%s',", procfs::get_process_name(getpid(), cstr));

    if (state->sa_ucontext == nullptr) {
        _LOGE("collect_crashing_thread: sa_ucontext is null");
        return;
    }

    const mcontext_t *mcontext = &(state->sa_ucontext->uc_mcontext);

    if (mcontext == nullptr) {
        _LOGE("collect_crashing_thread: mcontext is null");
        _EMIT(state, "}");
        return;
    }

    _EMIT(state, "'regs':[");

#if defined(__i386__)
    for (int i = 0; i < NGREG; i++) {
        _EMIT(state, "'r%0d':'0x%lu', ", i, mcontext->gregs[i]);
    }
    _EMIT(state, "'pc':'0x%lu', ", mcontext->gregs[REG_EIP]);
    _EMIT(state, "'sp':'0x%lu', ", mcontext->gregs[REG_ESP]);
    _EMIT(state, "],");
    _EMIT(state, "'signal':%ld, ", mcontext->gregs[REG_TRAPNO]);
    _EMIT(state, "'code':%ld, ", mcontext->gregs[REG_ERR]);

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    record_frame(mcontext->gregs[REG_EIP], state);

#elif defined(__x86_64__)
    for (int i = 0; i< NGREG; i++) {
        _EMIT(state, "'r%0d':'0x%ld', ", i, mcontext->gregs[i]);
    }
    _EMIT(state, "'pc':'0x%llu', ", mcontext->gregs[REG_RIP]);
    _EMIT(state, "'sp':'0x%llu', ", mcontext->gregs[REG_RSP]);
    _EMIT(state, "],");
    _EMIT(state, "'signal':%ld, ", mcontext->gregs[REG_TRAPNO]);
    _EMIT(state, "'code':%ld, ", mcontext->gregs[REG_ERR]);

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    record_frame(mcontext->gregs[REG_RIP], state);

#elif defined(__arm__)
    _EMIT(state, "'r00':0x%llu, ", REG_R0, mcontext->arm_r0);
    _EMIT(state, "'r01':0x%llu, ", REG_R1, mcontext->arm_r1);
    _EMIT(state, "'r02':0x%llu, ", REG_R2, mcontext->arm_r2);
    _EMIT(state, "'r03':0x%llu, ", REG_R3, mcontext->arm_r3);
    _EMIT(state, "'r04':0x%llu, ", REG_R4, mcontext->arm_r4);
    _EMIT(state, "'r05':0x%llu, ", REG_R5, mcontext->arm_r5);
    _EMIT(state, "'r06':0x%llu, ", REG_R6, mcontext->arm_r6);
    _EMIT(state, "'r07':0x%llu, ", REG_R7, mcontext->arm_r7);
    _EMIT(state, "'r08':0x%llu, ", REG_R8, mcontext->arm_r8);
    _EMIT(state, "'r09':0x%llu, ", REG_R9, mcontext->arm_r9);
    _EMIT(state, "'r10':0x%llu, ", REG_R10, mcontext->arm_r10);
    _EMIT(state, "'fp':0x%llu, ", mcontext->arm_fp);
    _EMIT(state, "'ip':0x%llu, ", mcontext->arm_ip);
    _EMIT(state, "'sp':'0x%llu', ", mcontext->arm_sp);
    _EMIT(state, "'lr':0x%llu, ", mcontext->arm_lr);
    _EMIT(state, "'pc':'0x%llu', ", mcontext->arm_pc);
    _EMIT(state, "'cpsr':'0x%llu', ", mcontext->arm_cpsr);
    _EMIT(state, "],");
    _EMIT(state, "'signal':%d, ", mcontext->trap_no);
    _EMIT(state, "'code':%d, ", mcontext->error_code);
    _EMIT(state, "'fault_address':'%p', ", mcontext->fault_address);

    _Unwind_SetGR(ucontext, REG_R0, mcontext->arm_r0);
    _Unwind_SetGR(ucontext, REG_R1, mcontext->arm_r1);
    _Unwind_SetGR(ucontext, REG_R2, mcontext->arm_r2);
    _Unwind_SetGR(ucontext, REG_R3, mcontext->arm_r3);
    _Unwind_SetGR(ucontext, REG_R4, mcontext->arm_r4);
    _Unwind_SetGR(ucontext, REG_R5, mcontext->arm_r5);
    _Unwind_SetGR(ucontext, REG_R6, mcontext->arm_r6);
    _Unwind_SetGR(ucontext, REG_R7, mcontext->arm_r7);
    _Unwind_SetGR(ucontext, REG_R8, mcontext->arm_r8);
    _Unwind_SetGR(ucontext, REG_R9, mcontext->arm_r9);
    _Unwind_SetGR(ucontext, REG_R10, mcontext->arm_r10);
    _Unwind_SetGR(ucontext, REG_R11, mcontext->arm_fp);
    _Unwind_SetGR(ucontext, REG_R12, mcontext->arm_ip);
    _Unwind_SetGR(ucontext, REG_R13, mcontext->arm_sp);
    _Unwind_SetGR(ucontext, REG_R14, mcontext->arm_lr);
    _Unwind_SetGR(ucontext, REG_R15, mcontext->arm_pc);
    // _Unwind_SetGR(ucontext, REG_IP, mcontext->arm_pc);
    // _Unwind_SetGR(ucontext, REG_SP, mcontext->arm_sp);

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    record_frame(mcontext->arm_ip, state);

#elif defined(__aarch64__)

    /* x0..x30 + sp + pc + pstate */
    for (int i = 0; i < NGREG; i++) {
        _EMIT(state, "r%d:0x%llu, ", i, mcontext->regs[i]);
    }
    _EMIT(state, "'sp':'0x%llu',", mcontext->sp);            // == r31
    _EMIT(state, "'pc':'0x%llu',", mcontext->pc);            // == r32
    _EMIT(state, "'pstate':'0x%llu',", mcontext->pstate);    // == r33
    _EMIT(state, "],");
    _EMIT(state, "fault_address':'%p', ", mcontext->fault_address);

    record_frame(mcontext->pc, state);

#else
    // FAIL

#endif // defined(__arm__)

#pragma clang diagnostic pop

}

void collect_noncrashing_threads(struct _Unwind_Context *ucontext, backtrace_t *state) {
    (void) ucontext;
    (void) state;

    // TODO
}

bool unwind_backtrace(char *backtrace_buffer, size_t max_size, const siginfo_t *siginfo,
                      const ucontext_t *sa_ucontext) {

    backtrace_t state = {};

    state.sa_ucontext = const_cast<ucontext_t *>(sa_ucontext);
    state.skip_frames = 1;
    state.frame_cnt = 0;
    state.siginfo = siginfo;
    state.cbuffer = "";

    _EMIT(&state, "'backtrace': {");

    // unwinds the backtrace and fills the buffer with stack frame addresses
    _Unwind_Backtrace(unwinder_cb, &state);

    _LOGD("[%s] unwind_backtrace: frames[%zu] skipped[%d] context[%p]",
          get_arch(), state.frame_cnt, state.skip_frames, state.sa_ucontext);

    // unwind the stack and capture all frames up to STACK_FRAMES_MAX,
    // serialized into the passed buffer
    _EMIT(&state, "'stackframes':[");
    for (size_t i = 0; i < state.frame_cnt; i++) {
        auto ip = state.frames[i];

        // Ignore null addresses that occur when the Link Register
        // is overwritten by the inner stack frames
        if (ip == 0) {
            continue;
        }

        // Ignore duplicate addresses that can occur with some compiler optimizations
        if (ip == state.frames[i - 1]) {
            continue;
        }

        // translate each stack frame
        transform_frame(i, ip, &state);
        _EMIT(&state, "%s", (i+1 < state.frame_cnt) ? "," : "");
    }

    _EMIT(&state, "]");  // stackframes: [
    _EMIT(&state, "}");  // backtrace: {

    size_t str_size = state.cbuffer.size();
    size_t copy_size = std::min(str_size, max_size - 2);
    memcpy(backtrace_buffer, state.cbuffer.data(), copy_size);
    backtrace_buffer[copy_size] = '\0';
    _LOGD("buffer[%zu]: %s", copy_size, backtrace_buffer);
    return copy_size == str_size;
}
