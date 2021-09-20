/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_PROCFS_H
#define _AGENT_NDK_PROCFS_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

namespace procfs {

    const char *get_process_name(pid_t, std::string &);

    const char *get_thread_name(pid_t, pid_t, std::string &);

    const char *get_thread_status_path(pid_t, pid_t, std::string &);

    const char *get_task_path(pid_t, std::string &);

    const char *get_thread_stat(pid_t, pid_t, std::string &);

}   // namespace procfs

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_PROCFS_H
