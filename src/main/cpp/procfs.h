/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_PROCFS_H
#define _AGENT_NDK_PROCFS_H

#include <string>

namespace procfs {

    const char *get_process_name(pid_t pid, std::string &processName);

    const char *get_thread_name(pid_t tid, std::string &threadName);

    const char *get_thread_status_path(pid_t pid, std::string &threadStatus);

    const char *get_task_path(pid_t pid, std::string& taskName);

}   // namespace procfs

#endif // _AGENT_NDK_PROCFS_H
