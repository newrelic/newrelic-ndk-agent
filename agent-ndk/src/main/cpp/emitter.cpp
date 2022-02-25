/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 **/

#include <unistd.h>
#include <dirent.h>
#include <string>
#include <sys/ucontext.h>
#include <asm/sigcontext.h>
#include <sstream>

#include <agent-ndk.h>
#include "backtrace.h"
#include "unwinder.h"
#include "procfs.h"
#include "signal-utils.h"
#include "jni/native-context.h"

/**
 * Append a formatted string to the output state
 * This method can only emit up to 2K characters
 *
 * @param state
 * @param fmt
 * @param vararg List of parameters appropriate to format
 */
static void _EMIT_F(std::string &state, const char *fmt...) {
    char buffer[2048];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    state.append(buffer);
}

/**
 * Append a list of strings to the output state
 *
 * @param state
 * @param fmt
 * @param vararg List of strings
 */
static std::string _EMIT_C(std::string &state, const char *cstr...) {
    va_list args;

    va_start(args, cstr);
    while (cstr != nullptr) {
        state.append(cstr);
        cstr = va_arg(args,
        const char*);
    }
    va_end(args);

    return state;
}

/**
 * Append a formatted element (as string) to the output state
 *
 * @param state
 * @param name of element (optional)
 * @param vararg List of element items
 */
static std::string _EMIT_E(std::string &state, const char *name, const char *cstr...) {
    va_list args;

    if (name != nullptr) {
        _EMIT_F(state, "'%s':", name);
    }
    state.append("{");
    va_start(args, cstr);
    while (cstr != nullptr) {
        state.append(cstr);
        cstr = va_arg(args,
        const char*);
        if (cstr != nullptr) {
            state.append(",");
        }
    }
    va_end(args);
    state.append("}");

    return state;
}

/**
 * Append a formatted array (as string) to the output state
 *
 * @param state
 * @param name of array (optional)
 * @param vararg List of array items
 */
static std::string _EMIT_A(std::string &state, const char *name, const char *cstr...) {
    va_list args;

    if (name != nullptr) {
        _EMIT_F(state, "'%s':", name);
    }
    state.append("[");
    va_start(args, cstr);
    while (cstr != nullptr) {
        state.append(cstr);
        cstr = va_arg(args,
        const char*);
        if (cstr != nullptr) {
            state.append(",");
        }
    }
    va_end(args);
    state.append("]");

    return state;
}

__unused static const char *frame_to_string(stackframe_t &stackframe, std::string &frame) {
    _EMIT_F(frame, "#%02zu pc %016x %s", stackframe.index, stackframe.pc, stackframe.so_path);
    if (*stackframe.sym_name != '\0') {
        _EMIT_F(frame, " (%s+%d)", stackframe.sym_name, stackframe.sym_addr_offset);
    }
    return frame.c_str();
}

static const char *frame_to_json(stackframe_t &stackframe, std::string &state) {
    std::string frame, cstr;

    _EMIT_F(frame, "'cstr':'%s',", frame_to_string(stackframe, cstr));
    _EMIT_F(frame, "'index':%d,", stackframe.index);
    _EMIT_F(frame, "'address':%zu,", stackframe.address);
    _EMIT_F(frame, "'pc':%zu,", stackframe.pc);
    _EMIT_F(frame, "'so_base':%zu,", stackframe.so_base);
    _EMIT_F(frame, "'sym_addr':%zu,", stackframe.sym_addr);
    _EMIT_F(frame, "'sym_addr_offset':%zu", stackframe.sym_addr_offset);

    if (*stackframe.so_path != '\0') {
        _EMIT_F(frame, ",'so_path':'%s'", stackframe.so_path);
    }
    if (*stackframe.sym_name != '\0') {
        _EMIT_F(frame, ",'sym_name':'%s'", stackframe.sym_name);
    }

    _EMIT_E(state, nullptr, frame.c_str(), nullptr);

    return state.c_str();
}

/**
 * Emit a single stack frame
 *
 * @param stackframe Single frame in the callstack
 * @param state Output buffer
 */
const char *emit_stackframe(stackframe_t &stackframe, std::string &frame) {
    return frame_to_json(stackframe, frame);
}

/**
 * Emit the violation's signal state
 *
 * @param siginfo_t Signal state
 * @param state Output buffer
 */
const char *emit_signal_context(const siginfo_t *siginfo, std::string &state) {
    std::string exception;

    _EMIT_F(exception, "'name':'%s',", "Native exception");
    if (siginfo != nullptr) {
        _EMIT_F(exception, "'cause':'%s',",
                sigutils::get_signal_description(siginfo->si_signo, siginfo->si_code));

        std::string csiginfo;
        _EMIT_F(csiginfo, "'signalName':'%s',",
                sigutils::get_signal_description(siginfo->si_signo, -1));
        _EMIT_F(csiginfo, "'signalCode':%d,", siginfo->si_code);
        _EMIT_F(csiginfo, "'faultAddress':%zu", siginfo->si_addr);

        _EMIT_E(exception, "signalInfo", csiginfo.c_str(), nullptr);
    }

    _EMIT_E(state, "exception", exception.c_str(), nullptr);

    return state.c_str();
}

/**
 * Emit the current register set state, which is contained
 * in the u_context's mcontext struct
 *
 * @param mcontext registers context from sa_context,
 * @param state Output buffer
 */
const char *emit_registers(const ucontext_t *sa_ucontext, std::string &state) {

    if (sa_ucontext == nullptr) {
        _LOGE("emit_registers: sa_ucontext is null");
        return nullptr;
    }

    // get pointer to registers specific context
    const mcontext_t *mcontext = &(sa_ucontext->uc_mcontext);
    if (mcontext == nullptr) {
        _LOGE("emit_registers: uc_mcontext is null");
        return nullptr;
    }

    std::string registers;

#if defined(__i386__)
    _EMIT_F(registers, "'eax':'%08lu',", mcontext->gregs[REG_EAX]);
    _EMIT_F(registers, "'ebx':'%08lu',", mcontext->gregs[REG_EBX]);
    _EMIT_F(registers, "'ecx':'%08lu',", mcontext->gregs[REG_ECX]);
    _EMIT_F(registers, "'edx':'%08lu',", mcontext->gregs[REG_EAX]);
    _EMIT_F(registers, "'edi':'%08lu',", mcontext->gregs[REG_EDI]);
    _EMIT_F(registers, "'esi':'%08lu',", mcontext->gregs[REG_ESI]);
    _EMIT_F(registers, "'ebp':'%08lu',", mcontext->gregs[REG_EBP]);
    _EMIT_F(registers, "'esp':'%08x',", mcontext->gregs[REG_ESP]);
    _EMIT_F(registers, "'eip':'%08x',", mcontext->gregs[REG_EIP]);
    _EMIT_F(registers, "'trapno':%d,", mcontext->gregs[REG_TRAPNO]);
    _EMIT_F(registers, "'error_code':%d", mcontext->gregs[REG_ERR]);

#elif defined(__x86_64__)
    for (int i = 0; i< NGREG; i++) {
        _EMIT_F(registers, "'r%d':'%016ld',", i, mcontext->gregs[i]);
    }
    _EMIT_F(registers, "'rip':'%016x',", mcontext->gregs[REG_RIP]);
    _EMIT_F(registers, "'rsp':'%016x',", mcontext->gregs[REG_RSP]);
    _EMIT_F(registers, "'trapno':%d,", mcontext->gregs[REG_TRAPNO]);
    _EMIT_F(registers, "'error_code':%d", mcontext->gregs[REG_ERR]);

#elif defined(__arm__)
    _EMIT_F(registers, "'r0':'%08x',", REG_R0, mcontext->arm_r0);
    _EMIT_F(registers, "'r1':'%08x',", REG_R1, mcontext->arm_r1);
    _EMIT_F(registers, "'r2':'%08x',", REG_R2, mcontext->arm_r2);
    _EMIT_F(registers, "'r3':'%08x',", REG_R3, mcontext->arm_r3);
    _EMIT_F(registers, "'r4':'%08x',", REG_R4, mcontext->arm_r4);
    _EMIT_F(registers, "'r5':'%08x',", REG_R5, mcontext->arm_r5);
    _EMIT_F(registers, "'r6':'%08x',", REG_R6, mcontext->arm_r6);
    _EMIT_F(registers, "'r7':'%08x',", REG_R7, mcontext->arm_r7);
    _EMIT_F(registers, "'r8':'%08x',", REG_R8, mcontext->arm_r8);
    _EMIT_F(registers, "'r9':'%08x',", REG_R9, mcontext->arm_r9);
    _EMIT_F(registers, "'r10':'%08x',", REG_R10, mcontext->arm_r10);
    _EMIT_F(registers, "'fp':'%08x',", mcontext->arm_fp);
    _EMIT_F(registers, "'ip':'%08x',", mcontext->arm_ip);
    _EMIT_F(registers, "'sp':'%08x',", mcontext->arm_sp);
    _EMIT_F(registers, "'lr':'%08x',", mcontext->arm_lr);
    _EMIT_F(registers, "'pc':'%08x',", mcontext->arm_pc);
    _EMIT_F(registers, "'cpsr':'%08x',", mcontext->arm_cpsr);
    _EMIT_F(registers, "'trapno':%d,", mcontext->trap_no);
    _EMIT_F(registers, "'error_code':%d,", mcontext->error_code);
    _EMIT_F(registers, "'fault_address':'%p'", mcontext->fault_address);

#elif defined(__aarch64__)
    for (int i = 0; i < 30; i++) {
        _EMIT_F(registers, "'x%d':'%016x',", i, mcontext->regs[i]);
    }
    _EMIT_F(registers, "'lr':'%016x',", mcontext->regs[30]);        // == r30
    _EMIT_F(registers, "'sp':'%016x',", mcontext->sp);              // == r31
    _EMIT_F(registers, "'pc':'%016x',", mcontext->pc);              // == r32
    _EMIT_F(registers, "'pst':'%016x',", mcontext->pstate);         // == r33
    _EMIT_F(registers, "'fault_address':'%016p'", mcontext->fault_address);

#else
    // FAIL
#endif // defined(__arm__)

    _EMIT_E(state, "registers", registers.c_str(), nullptr);

    return state.c_str();
}

/***
 * Emit the crashing current context: registers, crashing and other thread states.
 * Requires a current and valid ucontext containing the registers context (uc_mcontext).
 *
 */
const char *emit_context(backtrace_t &backtrace, std::string &state) {
    std::string cstr;
    jni::native_context_t &native_context = jni::get_native_context();

    _EMIT_F(state, "'name':'%s',", procfs::get_process_name(backtrace.pid, cstr));
    _EMIT_F(state, "'description':'%s',", backtrace.description);
    _EMIT_F(state, "'timestamp':%ld,", backtrace.timestamp);
    _EMIT_F(state, "'abi':'%s',", backtrace.arch);
    _EMIT_F(state, "'pid':%d,", backtrace.pid);
    _EMIT_F(state, "'ppid':%d,", backtrace.ppid);
    _EMIT_F(state, "'uid':%d,", backtrace.uid);
    _EMIT_F(state, "'buildid':'%s',", native_context.buildId);
    _EMIT_F(state, "'sessionid':'%s',", native_context.sessionId);
    _EMIT_F(state, "'platform':'%s'", "android");

    return state.c_str();
}

/**
 * Emit the stace trace (call stack) of the violation
 *
 * @param backtrace
 * @param state
 * @return callstack appended to state
 */
const char *emit_callstack(backtrace_state_t *backtrace_state, std::string &state) {
    std::string callstack;

    if (backtrace_state != nullptr) {
        for (size_t i = 0; i < backtrace_state->frame_cnt; i++) {
            std::string cstr;
            stackframe_t stackframe = {};
            transform_addr_to_stackframe(i, backtrace_state->frames[i], stackframe);
            _EMIT_C(callstack, emit_stackframe(stackframe, cstr), ",", nullptr);
        }
        if (!callstack.empty()) {
            callstack.pop_back();  // remove trailing comma
        }
    }

    _EMIT_A(state, "stack", callstack.c_str(), nullptr);

    return state.c_str();
}


/**
 * Emit the state of an individual thread, given a passed threadinfo_t
 *
 * @param thread Thread data
 * @return Thread data appended to state
 */
const char *emit_thread_info(threadinfo_t &thread, std::string &state) {
    std::string tstate, callstack;

    _EMIT_F(tstate, "'threadNumber':%d,", thread.tid);
    _EMIT_F(tstate, "'threadId':'%s',", thread.thread_name);
    _EMIT_F(tstate, "'state':'%s',", thread.thread_state);
    _EMIT_F(tstate, "'priority':%d,", thread.priority);
    _EMIT_F(tstate, "'crashed':%s,", thread.crashed ? "true" : "false");

    _EMIT_C(tstate, emit_callstack(thread.backtrace_state, callstack), nullptr);

    _EMIT_E(state, nullptr, tstate.c_str(), nullptr);

    return state.c_str();
}

/**
 * Collect the state of all threads in the process
 *
 * @param tid Thread ID
 * @return Threads state appended to state
 */
const char *emit_thread_state(backtrace_t &backtrace, std::string &state) {
    std::string threads;

    for (size_t i = 0; i < backtrace.threads.size(); i++) {
        std::string tstr;
        threadinfo_t &thread = backtrace.threads[i];
        _EMIT_C(threads, emit_thread_info(thread, tstr), ",", nullptr);
    }

    if (!threads.empty()) {
        threads.pop_back();  // remove training comma
    }

    _EMIT_A(state, "threads", threads.c_str(), nullptr);

    return state.c_str();
}

/**
 * Emit a fully formed report for this backtrace
 * @param backtrace
 * @param state Output buffer
 * @return const char* to string in output buffer
 */
const char *emit_backtrace(backtrace_t &backtrace, std::string &state) {
    std::string context, threads, regs, sig;

    state = "{";

    _EMIT_E(state, "backtrace",
            emit_context(backtrace, context),
            emit_registers(backtrace.state.sa_ucontext, regs),
            emit_signal_context(backtrace.state.siginfo, sig),
            emit_thread_state(backtrace, threads), nullptr);

    state.append("}");

    // translate single to double quotes
    std::replace(state.begin(), state.end(), '\'', '"');

    return state.c_str();
}
