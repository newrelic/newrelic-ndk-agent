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

/**
 * The JNIEnv is used for thread-local storage. For this reason, you cannot share a
 * JNIEnv between threads. Until a thread is attached, it has no JNIEnv, and cannot
 * make JNI calls. Calling AttachCurrentThread() on an already-attached thread is a no-op.
 * Threads attached through JNI must call DetachCurrentThread() before they exit.
 *
 * CheckJNI:
 * adb shell stop
 * adb shell setprop dalvik.vm.checkjni true
 * adb shell start
 */

namespace jni {

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Return native context instance
 * @return native_context singleton
 */
native_context_t &get_native_context() {
    static native_context_t instance = {};
    return instance;
}

/*
 * Release and reset any global JNI resources
 */
void release_native_context(JNIEnv *env) {
    native_context_t &native_context = get_native_context();

    pthread_mutex_lock(&lock);
    if (native_context.initialized) {
        release_delegate(env);

        pthread_mutex_unlock(&lock);
        pthread_mutex_destroy(&lock);

        lock = PTHREAD_MUTEX_INITIALIZER;
        native_context.initialized = false;

        return;
    }
    pthread_mutex_unlock(&lock);
}

/**
 *  If performance is important, it's useful to query certain values once and cache
 *  the results in native code. Because there is a limit of one JavaVM per process,
 *  it's reasonable to store this data in a static local structure.
 *
 *  Class references, field IDs, and method IDs are guaranteed valid until the class is unloaded.
 *  However, jclass is a class reference, but must be protected with a call to NewGlobalRef
 *
 * onLoad initialization:
 *  * Cache the JavaVM instance
 *  * Find class ID for delegate class(es)
 *  * Create an instance of delegate class(es)
 *  * Create a global reference to delegates to access from native threads
 *
 * Note:
 *  All allocated resources are never released by application
 *  The system will free all global refs when the process is terminated, and
 *  the pairing function JNI_OnUnload() is usually not called.
 *
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, __unused void *reserved) {
    jni::native_context_t native_context = jni::get_native_context();

    native_context.initialized = false;
    native_context.jvm = vm;

    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        _LOGE("JNI version 1.6 not supported");
        return JNI_ERR;     // JNI version not supported.
    }

    native_context.initialized = bind_delegate(env, native_context);

    if (!native_context.initialized) {
        _LOGE("Could not bind to JVM delegates. Reports will not be uploaded.");
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, __unused void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jni::release_native_context(env);
    }
}


bool env_check_and_clear_ex(JNIEnv *env) {
    if (env != nullptr) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return true;
        }
    }
    return false;
}

jclass env_find_class(JNIEnv *env, const char *class_name) {
    if (env != nullptr) {
        if (class_name != NULL) {
            jclass clz = env->FindClass(class_name);
            env_check_and_clear_ex(env);
            return clz;
        }
    }
    return nullptr;
}

jmethodID env_get_methodid(JNIEnv *env, jclass clz, const char *method_name, const char *method_sig) {
    if (env != nullptr) {
        if (clz != nullptr) {
            if (method_name != nullptr) {
                if (method_sig != nullptr) {
                    jmethodID methodId = env->GetMethodID(clz, method_name, method_sig);
                    env_check_and_clear_ex(env);
                    return methodId;
                }
            }
        }
    }
    return nullptr;
}

}   // namespace jni

