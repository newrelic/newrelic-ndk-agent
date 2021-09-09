/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>
#include <iostream>
#include <cstdlib>

#include <agent-ndk.h>
#include "unwinder.h"
#include "serializer.h"
#include "terminate-handler.h"

std::terminate_handler currentHandler = std::get_terminate();

/**
 * Report exception via agent HandledException
 */
static void terminateHandler() {
    char buffer[BACKTRACE_SZ_MAX];

    // reset the handler
    try {
        if (currentHandler != nullptr) {
            std::set_terminate(currentHandler);
        }
    } catch (...) {
        _LOGE("Couldn't reset the previous termination handler!");
    }

    try {
        std::exception_ptr unknown = std::current_exception();
        if (unknown != nullptr) {
            if (unwind_backtrace(buffer, sizeof(buffer), nullptr, nullptr)) {
                serializer::from_exception(buffer, sizeof(buffer));
            }
            _LOGI("Unknown current exception, rethrowing: %s", buffer);
            std::rethrow_exception(unknown);
        } else {
            _LOGI("Normal termination recvd");
        }
    } catch (const std::exception &e) {
        if (unwind_backtrace(buffer, sizeof(buffer), nullptr, nullptr)) {
            serializer::from_exception(buffer, sizeof(buffer));
        }
        _LOGI("Unexpected exception: %s %s", e.what(), buffer);
        throw e;

    } catch (...) {
        if (unwind_backtrace(buffer, sizeof(buffer), nullptr, nullptr)) {
            serializer::from_exception(buffer, sizeof(buffer));
        }
        _LOGI("Unknown exception: %s", buffer);
        throw;
    }

    // kill the process if the rethrown exception doesn't
    abort();
}

#if _LIBCPP_STD_VER <= 14
std::unexpected_handler currentUnexpectedHandler;

void unexpectedHandler() {
    // TODO
}

#endif // _LIBCPP_STD_VER <= 14


bool terminate_handler_initialize() {
    currentHandler = std::set_terminate(terminateHandler);
#if _LIBCPP_STD_VER <= 14
    currentUnexpectedHandler = std::set_unexpected(unexpectedHandler);
#endif // LIBCPP_STD_VER <= 14
    return true;
}

void terminate_handler_shutdown() {
    std::set_terminate(currentHandler);
#if _LIBCPP_STD_VER <= 14
    std::set_unexpected(currentUnexpectedHandler);
#endif // LIBCPP_STD_VER <= 14
}


