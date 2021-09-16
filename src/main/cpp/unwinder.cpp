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
#include <dirent.h>
#include <sys/ucontext.h>
#include <asm/sigcontext.h>
#include <sys/resource.h>

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

} backtrace_state_t;


void _EMIT(backtrace_state_t *state, const char *fmt...) {
    char buffer[0x400];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    std::string emit(buffer);
    std::replace(emit.begin(), emit.end(), '\'', '"');
    state->cbuffer.append(emit);
}

bool record_frame(uintptr_t ip, backtrace_state_t *state) {

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

void record_crashing_thread(backtrace_state_t *state) {

    if (state->sa_ucontext == nullptr) {
        _LOGE("record_crashing_thread: sa_ucontext is null");
        return;
    }

    // get pointer to machine specific context
    const mcontext_t *mcontext = &(state->sa_ucontext->uc_mcontext);
    if (mcontext == nullptr) {
        _LOGE("record_crashing_thread: mcontext is null");
        return;
    }

    // Program counter (pc)/instruction pointer (ip) register contains the crash address.
#if defined(__i386__)
    record_frame(mcontext->gregs[REG_EIP], state);

#elif defined(__x86_64__)// Program counter (pc)/instruction pointer (ip) register contains the crash address.
    record_frame(mcontext->gregs[REG_RIP], state);

#elif defined(__arm__)
    record_frame(mcontext->arm_ip, state);

#elif defined(__aarch64__)
    record_frame(mcontext->pc, state);

#else
    assert(0);  // fail

#endif // defined(__arm__)
}

void transform_frame(size_t index, const uintptr_t address, backtrace_state_t *state) {
    Dl_info info = {};
    std::string frame;

    if (dladdr(reinterpret_cast<void *>(address), &info)) {
        char buffer[0x100];

        std::snprintf(buffer, sizeof(buffer), "native: #%02zu pc", index);
        frame.append(buffer);

        std::snprintf(buffer, sizeof(buffer), " %016zu", address);
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

            uintptr_t load_addr = reinterpret_cast<uintptr_t>(info.dli_fbase);
            uintptr_t relative_addr = reinterpret_cast<uintptr_t>(address - load_addr);

            std::snprintf(buffer, sizeof(buffer), " + 0x%zu (0x%zu)", load_addr, relative_addr);
            frame.append(buffer);
        }
    }

    _LOGD("%s", frame.c_str());
    _EMIT(state, "'%s'", frame.c_str());
}


/***
 * Collect the crashing current context: registers, crashing and other thread states.
 * Requires a current and valid ucontext containing the machine context (uc_mcontext).
 *
 */
void collect_context(backtrace_state_t *state) {
    std::string cstr;

    _EMIT(state, "'ABI':'%s','pid':%d,'ppid':%d,'uid':%d,",
          get_arch(), getpid(), getppid(), getuid());
    _EMIT(state, "'platform':'%s',", "native");
    _EMIT(state, "'timestamp':%ld,", time(0L));
    _EMIT(state, "'name':'%s',", procfs::get_process_name(getpid(), cstr));

    if (state->sa_ucontext == nullptr) {
        _LOGE("collect_context: sa_ucontext is null");
        return;
    }

    // get pointer to machine specific context
    const mcontext_t *mcontext = &(state->sa_ucontext->uc_mcontext);
    if (mcontext == nullptr) {
        _LOGE("collect_context: mcontext is null");
        return;
    }

#if defined(__i386__)
    _EMIT(state, "'regs':{");
    for (int i = 0; i < NGREG; i++) {
        _EMIT(state, "'r%0d':'%08lu',", i, mcontext->gregs[i]);
    }
    _EMIT(state, "'pc':'%08x',", mcontext->gregs[REG_EIP]);
    _EMIT(state, "'sp':'%08x'", mcontext->gregs[REG_ESP]);
    _EMIT(state, "},");
    _EMIT(state, "'signal':%d,", mcontext->gregs[REG_TRAPNO]);
    _EMIT(state, "'code':%d", mcontext->gregs[REG_ERR]);

#elif defined(__x86_64__)
    _EMIT(state, "'regs':{");
    for (int i = 0; i< NGREG; i++) {
        _EMIT(state, "'r%d':'%016ld',", i, mcontext->gregs[i]);
    }
    _EMIT(state, "'pc':'%016x',", mcontext->gregs[REG_RIP]);
    _EMIT(state, "'sp':'%016x'", mcontext->gregs[REG_RSP]);
    _EMIT(state, "},");
    _EMIT(state, "'signal':%d,", mcontext->gregs[REG_TRAPNO]);
    _EMIT(state, "'code':%d", mcontext->gregs[REG_ERR]);

#elif defined(__arm__)
    _EMIT(state, "'regs':{");
    _EMIT(state, "'r0':%08x,", REG_R0, mcontext->arm_r0);
    _EMIT(state, "'r1':%08x,", REG_R1, mcontext->arm_r1);
    _EMIT(state, "'r2':%08x,", REG_R2, mcontext->arm_r2);
    _EMIT(state, "'r3':%08x,", REG_R3, mcontext->arm_r3);
    _EMIT(state, "'r4':%08x,", REG_R4, mcontext->arm_r4);
    _EMIT(state, "'r5':%08x,", REG_R5, mcontext->arm_r5);
    _EMIT(state, "'r6':%08x,", REG_R6, mcontext->arm_r6);
    _EMIT(state, "'r7':%08x,", REG_R7, mcontext->arm_r7);
    _EMIT(state, "'r8':%08x,", REG_R8, mcontext->arm_r8);
    _EMIT(state, "'r9':%08x,", REG_R9, mcontext->arm_r9);
    _EMIT(state, "'r10':%08x,", REG_R10, mcontext->arm_r10);
    _EMIT(state, "'fp':%08x,", mcontext->arm_fp);
    _EMIT(state, "'ip':%08x,", mcontext->arm_ip);
    _EMIT(state, "'sp':'%08x',", mcontext->arm_sp);
    _EMIT(state, "'lr':%08x,", mcontext->arm_lr);
    _EMIT(state, "'pc':'%08x',", mcontext->arm_pc);
    _EMIT(state, "'cpsr':'%08x'", mcontext->arm_cpsr);
    _EMIT(state, "},");
    _EMIT(state, "'signal':%d,", mcontext->trap_no);
    _EMIT(state, "'code':%d,", mcontext->error_code);
    _EMIT(state, "'fault_address':'%p'", mcontext->fault_address);

#elif defined(__aarch64__)
    _EMIT(state, "'regs':{");
    for (int i = 0; i < NGREG; i++) {
        _EMIT(state, "r%d:%016x,", i, mcontext->regs[i]);
    }
    _EMIT(state, "'sp':'%016x',", mcontext->sp);            // == r31
    _EMIT(state, "'pc':'%016x',", mcontext->pc);            // == r32
    _EMIT(state, "'pstate':'%016x'", mcontext->pstate);     // == r33
    _EMIT(state, "},");
    _EMIT(state, "fault_address':'%p'", mcontext->fault_address);

#else
    // FAIL

#endif // defined(__arm__)
}

const char *trim_trailing_ws(const char *buff) {
    char *buffEol = const_cast<char *>(&buff[std::strlen(buff) - 1]);
    while ((buffEol > buff) && (std::strpbrk(buffEol, "\n\r\t ") != nullptr)) {
        *buffEol-- = '\0';
    }
    return buff;
}

const char *parseThreadToken(const char *buff, const char *token) {
    size_t tokenLen = std::strlen(token);
    if (std::strncmp(buff, token, tokenLen) == 0) {
        const char *value = &buff[tokenLen];
        return trim_trailing_ws(value);
    }
    return nullptr;
}

void collect_thread_state(int tid, backtrace_state_t *state) {
    pid_t pid = getpid();
    std::string threadName;
    procfs::get_thread_name(pid, tid, threadName);

    _EMIT(state, "{");

    std::string cstr;
    FILE *fp = fopen(procfs::get_thread_status_path(pid, tid, cstr), "r");
    if (fp != nullptr) {
        char buff[1024];
        while (fgets(buff, sizeof(buff), fp) != NULL) {
            const char *value;
            if ((value = parseThreadToken(buff, "Name:\t")) != nullptr) {
                _EMIT(state, "'name':'%s',", value);
            } else if ((value = parseThreadToken(buff, "State:\t")) != nullptr) {
                _EMIT(state, "'state':'%s',", value);
            }
        }
        fclose(fp);
    }

    _EMIT(state, "'crashed':'%s',", tid == gettid() ? "true" : "false");
    _EMIT(state, "'priority':%d,", getpriority(PRIO_PROCESS, tid));
    _EMIT(state, "'schedstat':'%s',", procfs::get_thread_schedstat(pid, tid, cstr));
    _EMIT(state, "'threadid':%d", tid);


    _EMIT(state, "},");
}

void collect_threadinfo(backtrace_state_t *state) {
    struct dirent *_dirent;
    std::string path;
    const char *taskPath = procfs::get_task_path(getpid(), path);
    DIR *dir = opendir(taskPath);

    _EMIT(state, "'threads':[");

    // iterate through this process' threads gathering info on each thread
    while ((_dirent = readdir(dir)) != nullptr) {
        // only interested in numeric directories (representing thread ids)
        if (isdigit(_dirent->d_name[0])) {
            // convert dir entry name to thread id
            pid_t tid = std::strtol(_dirent->d_name, nullptr, 10);
            collect_thread_state(tid, state);
        }
    }
    state->cbuffer.pop_back();  // remove trailing comma
    closedir(dir);

    _EMIT(state, "]");  // 'threads')
}

void collect_stackframes(backtrace_state_t *state) {
    // unwind the stack and capture all frames up to STACK_FRAMES_MAX,
    // serialized into the passed buffer
    _EMIT(state, "'stackframes':[");
    for (size_t i = 0; i < state->frame_cnt; i++) {
        uintptr_t ip = state->frames[i];

        // Ignore null addresses that occur when the Link Register
        // is overwritten by the inner stack frames
        if (ip == 0) {
            continue;
        }

        // Ignore duplicate addresses that can occur with some compiler optimizations
        if (ip == state->frames[i - 1]) {
            continue;
        }

        // translate each stack frame
        transform_frame(i, ip, state);
        _EMIT(state, "%s", (i + 1 < state->frame_cnt) ? "," : "");
    }

    _EMIT(state, "]");  // stackframes: [
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

bool unwind_backtrace(char *backtrace_buffer, size_t max_size, const siginfo_t *siginfo,
                      const ucontext_t *sa_ucontext) {

    backtrace_state_t state = {};

    state.sa_ucontext = const_cast<ucontext_t *>(sa_ucontext);
    state.skip_frames = 0;
    state.frame_cnt = 0;
    state.siginfo = siginfo;
    state.cbuffer = "";

    _EMIT(&state, "{'backtrace':{");

    // unwinds the backtrace and fills the buffer with stack frame addresses
    _Unwind_Backtrace(unwinder_cb, &state);

    _LOGD("[%s] unwind_backtrace: frames[%zu] skipped[%d] context[%p]",
          get_arch(), state.frame_cnt, state.skip_frames, state.sa_ucontext);

    collect_context(&state);
    _EMIT(&state, ",");
    collect_threadinfo(&state);
    _EMIT(&state, ",");
    collect_stackframes(&state);

    _EMIT(&state, "}");  // backtrace: {
    _EMIT(&state, "}");  //

    size_t str_size = state.cbuffer.size();
    size_t copy_size = std::min(str_size, max_size - 2);
    memcpy(backtrace_buffer, state.cbuffer.data(), copy_size);
    backtrace_buffer[copy_size] = '\0';
    _LOGD("buffer[%zu]: %s", copy_size, backtrace_buffer);
    return copy_size == str_size;
}
