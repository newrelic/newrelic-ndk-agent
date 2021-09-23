/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>
#include <iostream>
#include <cstdlib>
#include <cxxabi.h>

#include <agent-ndk.h>
#include "unwinder.h"
#include "serializer.h"
#include "terminate-handler.h"

static std::terminate_handler currentHandler = std::get_terminate();

/* Module-wide mutex */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Report exception via agent HandledException
 */
static void terminateHandler() {
    try {
        std::exception_ptr exc = std::current_exception();

        std::type_info *tinfo = __cxxabiv1::__cxa_current_exception_type();
        char demangled[0x100];
        if (tinfo != NULL) {
            std::strncpy(demangled, (char *) tinfo->name(), sizeof(demangled));
        }

        if (exc != nullptr) {
            char *buffer = new char[BACKTRACE_SZ_MAX];
            if (unwind_backtrace(buffer, BACKTRACE_SZ_MAX, nullptr, nullptr)) {
                serializer::from_exception(buffer, std::strlen(buffer));
            }
            delete [] buffer;

            _LOGI("Unknown current exception, rethrowing: %s", buffer);
            std::rethrow_exception(exc);
        } else {
            _LOGI("Normal termination recvd");
        }
    } catch (const std::exception &e) {
        char *buffer = new char[BACKTRACE_SZ_MAX];
        if (unwind_backtrace(buffer, BACKTRACE_SZ_MAX, nullptr, nullptr)) {
            serializer::from_exception(buffer, std::strlen(buffer));
        }
        delete [] buffer;

        _LOGI("Unexpected exception: %s %s", e.what(), buffer);
        throw e;

    } catch (...) {
        char *buffer = new char[BACKTRACE_SZ_MAX];
        if (unwind_backtrace(buffer, BACKTRACE_SZ_MAX, nullptr, nullptr)) {
            serializer::from_exception(buffer, std::strlen(buffer));
        }
        delete [] buffer;

        _LOGI("Unknown exception: %s", buffer);
        throw;
    }

    // reset the handler
    try {
        if (currentHandler != nullptr) {
            std::set_terminate(currentHandler);
            currentHandler();
        }
    } catch (...) {
        _LOGE("Couldn't reset the previous termination handler!");
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
    pthread_mutex_lock(&mutex);
    currentHandler = std::set_terminate(terminateHandler);
#if _LIBCPP_STD_VER <= 14
    currentUnexpectedHandler = std::set_unexpected(unexpectedHandler);
#endif // LIBCPP_STD_VER <= 14
    pthread_mutex_unlock(&mutex);

    return true;
}

void terminate_handler_shutdown() {
    pthread_mutex_lock(&mutex);
    std::set_terminate(currentHandler);
#if _LIBCPP_STD_VER <= 14
    std::set_unexpected(currentUnexpectedHandler);
#endif // LIBCPP_STD_VER <= 14
    pthread_mutex_unlock(&mutex);
}


