/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>

#include <agent-ndk.h>
#include "jni/native-context.h"
#include "jni/jni-delegate.h"
#include "serializer.h"

namespace serializer {

    static std::string generateTmpFilename(const char *);

    void from_crash(const char *buffer, size_t buffsz) {
        to_storage("crash-", buffer, buffsz);
        // TODO crashes may be best left on storage and processed
        // on the next app launch
        jni::on_native_crash(buffer);
    }

    void from_exception(const char *buffer, size_t buffsz) {
        to_storage("ex-", buffer, buffsz);
        jni::on_native_exception(buffer);
    }

    void from_anr(const char *buffer, size_t buffsz) {
        to_storage("anr-", buffer, buffsz);
        jni::on_application_not_responding(buffer);
    }

    /**
     * Write the payload locally using MT thread-safe functions

     * @param payload
     * @param payload_size
     */
    bool to_storage(const char *filepath, const char *payload, size_t payload_size) {
        std::string storagePath = generateTmpFilename(filepath).c_str();
        std::ofstream os{storagePath.c_str(), std::ios::out | std::ios::binary};

        if (!os) {
            _LOGE("serializer::to_storage error %d: %s", errno, strerror(errno));
        } else {
            os.write(payload, payload_size);
            os.flush();
            os.close();
            _LOGD("Native report written to [%s]", storagePath.c_str());
            return true;
        }

        return false;
    }

    static std::string generateTmpFilename(const char *filePrefix) {
        jni::native_context_t &native_context = jni::get_native_context();
        std::ostringstream oss;
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto timer = system_clock::to_time_t(now);
        std::tm wallTime = *std::localtime(&timer);

        oss << native_context.reportPathAbsolute << "/"
            << filePrefix
            << std::put_time(&wallTime, "%Y%m%d%H%M%S")
            << std::setfill('0')
            << std::setw(3)
            << ms.count();

        return oss.str();
    }

}   // namespace serializer