/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <inttypes.h>
#include <android/log.h>
#include <unistd.h>
#include <pthread.h>

#include <agent-ndk.h>
#include "native-context.h"
#include "jni.h"
#include "jni-delegate.h"

namespace jni {

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    native_context_t &get_native_context() {
        static native_context_t instance = {};
        return instance;
    }

    native_context_t &set_native_context(JNIEnv *env, jobject managedContext) {
        static native_context_t &instance = get_native_context();

        if (managedContext != nullptr) {
            jni::native_context_t &native_context = jni::get_native_context();

            jclass managedContextClass = jni::env_get_object_class(env, managedContext);
            jclass fileClass = static_cast<jclass>(jni::env_find_class(env, "java/io/File"));
            jmethodID absolutePathMethodId = jni::env_get_methodid(env,
                                                                   fileClass,
                                                                   "getAbsolutePath",
                                                                   "()Ljava/lang/String;");

            // copy the report path field
            jfieldID reportsDir = jni::env_get_fieldid(env,
                                                       managedContextClass,
                                                       "reportsDir",
                                                       "Ljava/io/File;");

            jobject fileObject = jni::env_get_object_field(env, managedContext, reportsDir);
            jobject pathObject = jni::env_call_object_method(env, fileObject, absolutePathMethodId);
            const char *reportsPath = jni::env_get_string_UTF_chars(env,
                                                                    static_cast<jstring>(pathObject));

            std::strncpy(native_context.reportPathAbsolute, reportsPath,
                         sizeof(native_context.reportPathAbsolute));


            // copy the session ID field
            jfieldID fieldId = jni::env_get_fieldid(env,
                                                    managedContextClass,
                                                    "sessionId",
                                                    "Ljava/lang/String;");
            jobject fieldObject = jni::env_get_object_field(env, managedContext, fieldId);
            const char *sessionId = jni::env_get_string_UTF_chars(env,
                                                                  static_cast<jstring>(fieldObject));
            std::strncpy(native_context.sessionId, sessionId, sizeof(native_context.sessionId));

            // copy the session ID field
            fieldId = jni::env_get_fieldid(env,
                                           managedContextClass,
                                           "buildId",
                                           "Ljava/lang/String;");
            fieldObject = jni::env_get_object_field(env, managedContext, fieldId);
            const char *buildId = jni::env_get_string_UTF_chars(env,
                                                                static_cast<jstring>(fieldObject));
            std::strncpy(native_context.buildId, buildId, sizeof(native_context.buildId));

        }

        return instance;
    }

    /*
     * Release and reset any global JNI resources
     */
    void release_native_context(JNIEnv *env, native_context_t &native_context) {
        pthread_mutex_lock(&lock);
        if (native_context.initialized) {
            release_delegate(env, native_context);

            pthread_mutex_unlock(&lock);
            pthread_mutex_destroy(&lock);

            lock = PTHREAD_MUTEX_INITIALIZER;
            native_context.initialized = false;

            return;
        }
        pthread_mutex_unlock(&lock);
    }

}   // namespace jni

