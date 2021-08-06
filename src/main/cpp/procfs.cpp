/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <string>
#include <agent-ndk.h>

#include "procfs.h"

namespace procfs {

// https://www.kernel.org/doc/Documentation/filesystems/proc.txt

const char *get_process_name(pid_t pid, std::string &processName) {
    char path[PATH_MAX];
    char processNameBuffer[1024];
    FILE *fp = nullptr;

    std::snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    if ((fp = fopen(path, "r"))) {
        processName = fgets(processNameBuffer, sizeof(processNameBuffer), fp);
        fclose(fp);
    }

    return processName.c_str();
}

const char *get_thread_name(pid_t pid, std::string &threadName) {
    char path[PATH_MAX];
    char threadNameBuffer[1024];
    char *threadname = nullptr;
    FILE *fp = nullptr;

    std::snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    if ((fp = fopen(path, "r"))) {
        threadname = fgets(threadNameBuffer, sizeof(threadNameBuffer), fp);
        fclose(fp);
        if (threadname != nullptr) {
            size_t len = strlen(threadname);
            if ((len > 0) && (threadname[len - 1] == '\n')) {
                threadname[len - 1] = '\0';
            }
        }
    }

    threadName = (threadname ? threadname : "<unknown>");

    return threadName.c_str();
}

const char *get_thread_status_path(pid_t pid, std::string &threadStatus) {
    char statusBuffer[PATH_MAX];
    std::snprintf(statusBuffer, sizeof(statusBuffer), "/proc/%d/status", pid);
    threadStatus = statusBuffer;

    return threadStatus.c_str();
}

const char *get_task_path(pid_t pid, std::string& taskPath) {
    char pathBuffer[PATH_MAX];

    std::snprintf(pathBuffer, sizeof(pathBuffer), "/proc/%d/task", pid);
    taskPath = pathBuffer;

    return taskPath.c_str();
}

}   // namespace procfs
