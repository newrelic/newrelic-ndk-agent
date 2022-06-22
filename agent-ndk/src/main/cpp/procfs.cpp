/**
 * Copyright 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <errno.h>
#include <string>

#include <agent-ndk.h>
#include "procfs.h"

namespace procfs {

    // https://www.kernel.org/doc/Documentation/filesystems/proc.txt

    const char *trim_trailing_ws(const char *buff) {
        char *buffEol = const_cast<char *>(&buff[std::strlen(buff) - 1]);
        while ((buffEol > buff) && (std::strpbrk(buffEol, "\n\r\t ") != nullptr)) {
            *buffEol-- = '\0';
        }
        return buff;
    }

    const char *get_process_name(pid_t pid, std::string &processName) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

        processName = "<unknown>";
        FILE *fp = fopen(path, "r");
        if (fp != nullptr) {
            char buff[1024];
            if (fgets(buff, sizeof(buff), fp)) {
                processName = trim_trailing_ws(buff);
            }
            fclose(fp);
        } else {
            _LOGE("get_process_name: error[%d]: %s", errno, strerror(errno));
        }

        return processName.c_str();
    }

    const char *get_thread_name(pid_t pid, pid_t tid, std::string &threadName) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/task/%d/comm", pid, tid);

        threadName = "<unknown>";
        FILE *fp = fopen(path, "r");
        if (fp != nullptr) {
            char buff[1024];
            if (fgets(buff, sizeof(buff), fp) != nullptr) {
                threadName = trim_trailing_ws(buff);
            }
            fclose(fp);
        } else {
            _LOGE("get_thread_name: error[%d]: %s", errno, strerror(errno));
        }

        return threadName.c_str();
    }

    const char *get_thread_status_path(pid_t pid, pid_t tid, std::string &threadStatus) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/task/%d/status", pid, tid);
        threadStatus = path;

        return threadStatus.c_str();
    }

    const char *get_thread_stat(pid_t pid, pid_t tid, std::string &stat) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/task/%d/stat", pid, tid);

        FILE *fp = fopen(path, "r");
        if (fp != nullptr) {
            char buff[1024];
            if (fgets(buff, sizeof(buff), fp) != nullptr) {
                stat = trim_trailing_ws(buff);
            }
            fclose(fp);
        } else {
            _LOGE("get_thread_stat: error[%d]: %s", errno, strerror(errno));
        }

        return stat.c_str();
    }

    const char *get_task_path(pid_t pid, std::string &taskPath) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/task", pid);
        taskPath = path;

        return taskPath.c_str();
    }

    const char *get_process_stat(pid_t pid, std::string &statResult) {
        char path[PATH_MAX];
        std::snprintf(path, sizeof(path), "/proc/%d/stat", pid);

        FILE *fp = fopen(path, "r");
        if (fp != nullptr) {
            char buff[1024];
            if (fgets(buff, sizeof(buff), fp) != nullptr) {
                statResult = trim_trailing_ws(buff);
            }
            fclose(fp);
        } else {
            _LOGE("get_cpu_sample: error[%d]: %s", errno, strerror(errno));
        }

        return statResult.c_str();
    }

}   // namespace procfs
