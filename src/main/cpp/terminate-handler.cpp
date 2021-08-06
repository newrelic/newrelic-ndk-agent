/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>
#include <iostream>
#include <cstdlib>
#include <agent-ndk.h>
#include "unwinder.h"

std::terminate_handler currentHandler = std::get_terminate();

/**
 * Report exception via agent HandledException
 */
static void terminateHandler() {
    char buffer[BACKTRACE_SZ_MAX];

    try {
        std::exception_ptr unknown = std::current_exception();
        if (unknown != nullptr) {
            collectBacktrace(buffer, sizeof(buffer), nullptr, nullptr);
            _LOGI("Unknown current exception, rethrowing: %s", buffer);
            std::rethrow_exception(unknown);
        } else {
            _LOGI("Normal termination");
        }
    } catch (const std::exception &e) {
        collectBacktrace(buffer, sizeof(buffer), nullptr, nullptr);
        _LOGI("Unexpected exception: %s %s", e.what(), buffer);
        throw e;

    } catch (...) {
        collectBacktrace(buffer, sizeof(buffer), nullptr, nullptr);
        _LOGI("Unknown exception: %s", buffer);
        throw;
    };
    
    abort();
}

#if _LIBCPP_STD_VER <= 14
std::unexpected_handler currentUnexpectedHandler;

void unexpectedHandler() {
    // TODO
}
#endif // _LIBCPP_STD_VER <= 14

void setTerminateHandler() {
    currentHandler = std::set_terminate(terminateHandler);
#if _LIBCPP_STD_VER <= 14
    currentUnexpectedHandler = std::set_unexpected(unexpectedHandler);
#endif // LIBCPP_STD_VER <= 14
}

