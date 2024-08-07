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

    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
                         sizeof(native_context.reportPathAbsolute) - 1);


            // copy the session ID field
            jfieldID fieldId = jni::env_get_fieldid(env,
                                                    managedContextClass,
                                                    "sessionId",
                                                    "Ljava/lang/String;");
            jobject fieldObject = jni::env_get_object_field(env, managedContext, fieldId);
            const char *sessionId = jni::env_get_string_UTF_chars(env,
                                                                  static_cast<jstring>(fieldObject));
            std::strncpy(native_context.sessionId, sessionId, sizeof(native_context.sessionId) - 1);

            // copy the build Id field
            fieldId = jni::env_get_fieldid(env,
                                           managedContextClass,
                                           "buildId",
                                           "Ljava/lang/String;");
            fieldObject = jni::env_get_object_field(env, managedContext, fieldId);
            const char *buildId = jni::env_get_string_UTF_chars(env,
                                                                static_cast<jstring>(fieldObject));
            std::strncpy(native_context.buildId, buildId, sizeof(native_context.buildId) - 1);

            // copy the anr monitor field

            fieldId = jni::env_get_fieldid(env,
                                           managedContextClass,
                                           "anrMonitor",
                                           "Z");
            jboolean anrMonitorEnabled = jni::env_get_boolean_field(env, managedContext, fieldId);
            native_context.anrMonitorEnabled = anrMonitorEnabled;
        }

        return instance;
    }

    /*
     * Release and reset any global JNI resources
     */
    void release_native_context(JNIEnv *env, native_context_t &native_context) {
        if (0 == pthread_mutex_lock(&mutex)) {
            if (native_context.initialized) {
                release_delegate(env, native_context);

                if (0 != pthread_mutex_unlock(&mutex)) {
                    _LOGE_POSIX("pthread_mutex_unlock()");
                }

                /* FIXME "FORTIFY: pthread_mutex_lock called on a destroyed mutex"
                if (0 != pthread_mutex_destroy(&mutex)) {
                    _LOGE_POSIX("pthread_mutex_destroy()");
                }
                mutex = PTHREAD_MUTEX_INITIALIZER;
                */

                native_context.initialized = false;

                return;
            }
            if (0 != pthread_mutex_unlock(&mutex)) {
                _LOGE_POSIX("pthread_mutex_unlock()");
            }
        } else {
            _LOGE_POSIX("pthread_mutex_lock()");
        }
    }

}   // namespace jni

