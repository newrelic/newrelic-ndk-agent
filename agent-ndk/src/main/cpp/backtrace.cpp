/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <cstdlib>
#include <vector>

#include <agent-ndk.h>
#include "procfs.h"
#include "backtrace.h"
#include "unwinder.h"
#include "emitter.h"
#include "signal-utils.h"


void collect_thread_info(int tid, threadinfo_t &threadinfo) {
    pid_t pid = getpid();
    std::string cstr;
    const char *tstat = procfs::get_thread_stat(pid, tid, cstr);

    threadinfo.tid = tid;

    // everything needed is in thread's /proc stat file
    if (tstat && *tstat != '\0') {
        char value[0x100];
        const char *delim = " ";
        char *ppos = const_cast<char *>(tstat);
        char *token = strtok_r(ppos, delim, &ppos);

        if (token) {
            token = strtok_r(nullptr, ")", &ppos);
            if (token) {
                if (std::sscanf(token, "(%[A-Za-z0-9 _.:-])", value) == 1) {
                    std::strncpy(threadinfo.thread_name, value, sizeof(threadinfo.thread_name));
                }
            }
            token = strtok_r(nullptr, delim, &ppos);
            if (token) {
                const char *rstate = "unknown";
                switch (std::tolower(*token)) {
                    case 'r':
                        rstate = "RUNNING";
                        break;
                    case 's':
                    case 'd':
                        rstate = "SLEEPING";
                        break;
                    case 'z':
                        rstate = "ZOMBIE";
                        break;
                    case 't':
                        rstate = "STOPPED";
                        break;
                    case 'x':
                        rstate = "DEAD";
                        break;
                    case 'w':
                        rstate = "WAKING";
                        break;
                    case 'k':
                        rstate = "WAKE KILL";
                        break;
                    case 'p':
                        rstate = "PARKED";
                        break;
                };  // switch
                std::strncpy(threadinfo.thread_state, rstate, sizeof(threadinfo.thread_state));
            }
            token = strtok_r(nullptr, delim, &ppos);    // skip ppid
            token = strtok_r(nullptr, delim, &ppos);    // skip pgrp
            token = strtok_r(nullptr, delim, &ppos);    // skip session id
            token = strtok_r(nullptr, delim, &ppos);    // skip tty_nr
            token = strtok_r(nullptr, delim, &ppos);    // skip tty_pgrp
            token = strtok_r(nullptr, delim, &ppos);    // skip flags
            token = strtok_r(nullptr, delim, &ppos);    // skip min_fault
            token = strtok_r(nullptr, delim, &ppos);    // skip cmin_fault
            token = strtok_r(nullptr, delim, &ppos);    // skip maj_fault
            token = strtok_r(nullptr, delim, &ppos);    // skip cmaj_fault
            token = strtok_r(nullptr, delim, &ppos);    // skip utime
            token = strtok_r(nullptr, delim, &ppos);    // skip stime
            token = strtok_r(nullptr, delim, &ppos);    // skip cutime
            token = strtok_r(nullptr, delim, &ppos);    // skip cstime
            token = strtok_r(nullptr, delim, &ppos);    // prior
            if (token) {
                threadinfo.priority = std::strtol(token, nullptr, 10);
            }
            token = strtok_r(nullptr, delim, &ppos);    // skip nice
            token = strtok_r(nullptr, delim, &ppos);    // skip num threads
            token = strtok_r(nullptr, delim, &ppos);    // skip itrealvalue
            token = strtok_r(nullptr, delim, &ppos);    // skip start time
            token = strtok_r(nullptr, delim, &ppos);    // skip vsize
            token = strtok_r(nullptr, delim, &ppos);    // skip rss mem
            token = strtok_r(nullptr, delim, &ppos);    // skip rss lim
            token = strtok_r(nullptr, delim, &ppos);    // skip start code
            token = strtok_r(nullptr, delim, &ppos);    // skip end code
            token = strtok_r(nullptr, delim, &ppos);    // start stack
            if (token) {
                threadinfo.stack = std::strtoull(token, nullptr, 10);
            }
            // ignore the rest, the values tend to be the same for all threads anyway
        }
    }

    threadinfo.crashed = (tid == gettid());

    // TODO threadinfo.stackframes[]
}

void collect_thread_state(std::vector <threadinfo_t>& threads) {
    std::string path;
    const char *taskPath = procfs::get_task_path(getpid(), path);

    DIR *dir = opendir(taskPath);
    if (dir != nullptr) {
        // iterate through this process' threads gathering info on each thread
        struct dirent *_dirent;
        while ((_dirent = readdir(dir)) != nullptr &&
               (threads.size() < BACKTRACE_THREADS_MAX)) {
            // only interested in numeric directories (representing thread ids)
            if (isdigit(_dirent->d_name[0])) {
                pid_t tid = std::strtol(_dirent->d_name, nullptr, 10);
                threadinfo_t threadinfo = {};
                collect_thread_info(tid, threadinfo);
                threads.push_back(threadinfo);
            }
        }
        closedir(dir);
    }
}

bool collect_backtrace(char *backtrace_buffer,
                       size_t max_size,
                       const siginfo_t *siginfo,
                       const ucontext_t *sa_ucontext) {

    std::string state; // , cstr;
    backtrace_t backtrace = {};

    backtrace.state.sa_ucontext = sa_ucontext;
    backtrace.state.siginfo = siginfo;

    std::strncpy(backtrace.arch, get_arch(), sizeof(backtrace.arch));
    std::strncpy(backtrace.description,
                 sigutils::get_signal_description(siginfo->si_signo, siginfo->si_code),
                 sizeof(backtrace.description));

    backtrace.timestamp = time(0L);
    backtrace.uid = getuid();
    backtrace.pid = getpid();
    backtrace.ppid = getppid();
    backtrace.threads.clear();

    unwind_backtrace(backtrace.state);
    collect_thread_state(backtrace.threads);

    std::string emitted = emit_backtrace(backtrace, state);

    size_t str_size = emitted.size();
    size_t copy_size = std::min(str_size, max_size - 2);
    memcpy(backtrace_buffer, emitted.data(), copy_size);
    backtrace_buffer[copy_size] = '\0';
    _LOGD("buffer[%zu]: %s", copy_size, backtrace_buffer);

    return copy_size == str_size;
}
