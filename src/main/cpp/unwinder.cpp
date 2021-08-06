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

// TODO Emit data in final format
#define  _EMIT(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

typedef struct backtrace {
    uintptr_t frames[BACKTRACE_FRAMES_MAX];
    size_t frame_cnt;
    int skip_frames;
    ucontext_t *sa_ucontext;
    const siginfo_t *siginfo;

} backtrace_t;

// forward decls
void collectCrashingThread(struct _Unwind_Context *, backtrace_t *);
void collectNoncrashingThreads(struct _Unwind_Context *, backtrace_t *);
bool recordFrame(uintptr_t, backtrace_t *);
void transformFrame(size_t, const _Unwind_Ptr, std::string *);

_Unwind_Reason_Code unwinder_cb(struct _Unwind_Context *ucontext, void *arg) {
    backtrace_t *state = static_cast<backtrace_t *>(arg);

    if (state->frame_cnt == 0) {
        collectCrashingThread(ucontext, state);
        return _URC_NO_REASON;
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

    return recordFrame(ip, state) ? _URC_NO_REASON : _URC_END_OF_STACK;
}

bool recordFrame(uintptr_t ip, backtrace_t *state) {

    if (state->frame_cnt >= BACKTRACE_FRAMES_MAX) {
        _LOGE("recordFrame: stack is full");
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
            _LOGE("recordFrame: ip is null or duplicate of frame[%lu]", state->frame_cnt - 1);
#pragma clang diagnostic pop
            return true;
        }
    }

    // Finally add the address to the storage.
    state->frames[state->frame_cnt++] = ip;

    return true;
}

void transformFrame(size_t index, const _Unwind_Ptr address, std::string *backtrace) {
    Dl_info info = {};
    std::string frame;

    if (dladdr(reinterpret_cast<void *>(address), &info)) {
        char buffer[0x100];

        std::snprintf(buffer, sizeof(buffer), "#%02zu pc", index);
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

    _EMIT("    %s, ", frame.c_str());
    backtrace->append(frame);
    backtrace->append("\\n");
}

/***
 * Register context
 *
 */
void collectContext(struct _Unwind_Context *ucontext, backtrace_t *state) {
    (void) ucontext;
    (void) state;

    // TODO
}

void collectCrashingThread(struct _Unwind_Context *ucontext, backtrace_t *state) {
    (void) ucontext;
    std::string frame;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

    _EMIT("{'ABI':'%s', 'pid':%d, 'ppid':%d, 'tid':%d, 'uid':%d",
          get_arch(), getpid(), getppid(), gettid(), getuid());
    _EMIT("'timestamp':%ld, ", time(0L));
    _EMIT("'name':%s, ", "FIXME");

    if (state->sa_ucontext == nullptr) {
        _LOGE("collectCrashingThread: sa_ucontext is null");
        _EMIT("}");
        return;
    }

    const mcontext_t *mcontext = &(state->sa_ucontext->uc_mcontext);

    if (mcontext == nullptr) {
        _LOGE("collectCrashingThread: mcontext is null");
        _EMIT("}");
        return;
    }

    _EMIT("'regs':[ ");

#if defined(__i386__)
    for (int i = 0; i< NGREG; i++) {
        _EMIT("'r%d':'0x%lu', ", i, mcontext->gregs[i]);
    }
    _EMIT("'pc':'0x%lu', ", mcontext->gregs[REG_EIP]);
    _EMIT("'sp':'0x%lu', ", mcontext->gregs[REG_ESP]);
    _EMIT("],");
    _EMIT("'signal':%ld, ", mcontext->gregs[REG_TRAPNO]);
    _EMIT("'code':%ld, ", mcontext->gregs[REG_ERR]);

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    recordFrame(mcontext->gregs[REG_EIP], state);

#elif defined(__x86_64__)
    for (int i = 0; i< NGREG; i++) {
        _EMIT("'r%d':'0x%ld', ", i, mcontext->gregs[i]);
    }
    _EMIT("'pc':'0x%llu', ", mcontext->gregs[REG_RIP]);
    _EMIT("'sp':'0x%llu', ", mcontext->gregs[REG_RSP]);
    _EMIT("],");
    _EMIT("'signal':%ld, ", mcontext->gregs[REG_TRAPNO]);
    _EMIT("'code':%ld, ", mcontext->gregs[REG_ERR]);

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    recordFrame(mcontext->gregs[REG_RIP], state);

#elif defined(__arm__)
    _EMIT("'r0': 0x%llu, ", REG_R0, mcontext->arm_r0);
    _EMIT("'r1': 0x%llu, ", REG_R1, mcontext->arm_r1);
    _EMIT("'r2': 0x%llu, ", REG_R2, mcontext->arm_r2);
    _EMIT("'r3': 0x%llu, ", REG_R3, mcontext->arm_r3);
    _EMIT("'r4': 0x%llu, ", REG_R4, mcontext->arm_r4);
    _EMIT("'r5': 0x%llu, ", REG_R5, mcontext->arm_r5);
    _EMIT("'r6': 0x%llu, ", REG_R6, mcontext->arm_r6);
    _EMIT("'r7': 0x%llu, ", REG_R7, mcontext->arm_r7);
    _EMIT("'r8': 0x%llu, ", REG_R8, mcontext->arm_r8);
    _EMIT("'r9': 0x%llu, ", REG_R9, mcontext->arm_r9);
    _EMIT("'r10': 0x%llu, ", REG_R10, mcontext->arm_r10);
    _EMIT("'fp': 0x%llu, ", mcontext->arm_fp);
    _EMIT("'ip': 0x%llu, ", mcontext->arm_ip);
    _EMIT("'sp':'0x%llu', ", mcontext->arm_sp);
    _EMIT("'lr': 0x%llu, ", mcontext->arm_lr);
    _EMIT("'pc':'0x%llu', ", mcontext->arm_pc);
    _EMIT("'cpsr':'0x%llu', ", mcontext->arm_cpsr);
    _EMIT("],");
    _EMIT("'signal':%d, ", mcontext->trap_no);
    _EMIT("'code':%d, ", mcontext->error_code);
    _EMIT("'fault_address':'%p', ", mcontext->fault_address);

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

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
    recordFrame(mcontext->arm_ip, state);

#elif defined(__aarch64__)

    /* x0..x30 + sp + pc + pstate */
    for (int i = 0; i < NGREG; i++) {
        _EMIT("r%d: 0x%llu, ", i, mcontext->regs[i]);
    }
    _EMIT("'sp': '0x%llu', ", mcontext->sp);            // == r31
    _EMIT("'pc': '0x%llu', ", mcontext->pc);            // == r32
    _EMIT("'pstate': '0x%llu', ", mcontext->pstate);    // == r33
    _EMIT("],");
    _EMIT("fault_address': '%p', ", mcontext->fault_address);

    recordFrame(mcontext->pc, state);

#else
    // FAIL

#endif // defined(__arm__)

#pragma clang diagnostic pop

    _EMIT("}");
}

void collectNoncrashingThreads(struct _Unwind_Context *ucontext, backtrace_t *state) {
    (void) ucontext;
    (void) state;

    // TODO
}

bool collectBacktrace(char *backtrace_buffer, size_t max_size, const siginfo_t *siginfo,
                      const ucontext_t *sa_ucontext) {

    std::string backtrace;
    backtrace_t state = {};

    state.sa_ucontext = const_cast<ucontext_t *>(sa_ucontext);
    state.skip_frames = 1;
    state.frame_cnt = 0;
    state.siginfo = siginfo;

    // unwinds the backtrace and fills the buffer with stack frame addresses
    _Unwind_Backtrace(unwinder_cb, &state);

    _LOGD("[%s] collectBacktrace: frames[%zu] skipped[%d] context[%p]",
        get_arch(), state.frame_cnt, state.skip_frames, state.sa_ucontext);

    backtrace.append("'backtrace': {");

    // unwind the stack and capture all frames up to STACK_FRAMES_MAX,
    // serialized into the passed buffer
    for (size_t idx = 0; idx < state.frame_cnt; ++idx) {
        auto ip = state.frames[idx];

        // Ignore null addresses that occur when the Link Register
        // is overwritten by the inner stack frames
        if (ip == 0) {
            continue;
        }

        // Ignore duplicate addresses that can occur with some compiler optimizations
        if (ip == state.frames[idx - 1]) {
            continue;
        }

        // translate each stack frame
        transformFrame(idx, ip, &backtrace);
    }

    size_t str_size = backtrace.size();
    size_t copy_size = std::min(str_size, max_size - 2);
    memcpy(backtrace_buffer, backtrace.data(), copy_size);

    backtrace.append("}");
    backtrace_buffer[backtrace.size()] = '\0';

    return copy_size == str_size;
}
