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
#include "jni-delegate.h"

namespace jni {

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    native_context_t &get_native_context() {
        static native_context_t instance = {};
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

