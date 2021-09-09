/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <pthread.h>
#include <agent-ndk.h>

#include "native-context.h"

/**
 * https://developer.android.com/training/articles/perf-jni#general-tips
 */

static const char *delegate_class = "com/newrelic/agent/android/ndk/JNIDelegate";
static const char *on_native_crashMethod = "onNativeCrash";
static const char *on_native_exceptionMethod = "onNativeException";
static const char *on_application_not_respondingMethod = "onApplicationNotResponding";


bool bind_delegate(JNIEnv *env, jni::native_context_t &native_context) {
    jclass jniDelegate = env->FindClass(delegate_class);
    if (jniDelegate == nullptr) {
        _LOGE("Unable to find delegate class [%s]", delegate_class);
        return JNI_ERR;
    }

    jclass delegateClassRef = static_cast<jclass>(env->NewGlobalRef(jniDelegate));
    jmethodID delegateCtor = env->GetMethodID(delegateClassRef, "<init>", "()V");
    jobject delegateObj = env->NewObject(delegateClassRef, delegateCtor);

    native_context.jniDelegateClass = delegateClassRef;
    native_context.jniDelegateObject = env->NewGlobalRef(delegateObj);

    // set the delegate method IDs (opaque structs that should not be cast to jobject)
    // can this be cached?
    native_context.on_native_crash  = env->GetMethodID(
            native_context.jniDelegateClass,
            on_native_crashMethod, "(Ljava/lang/String;)V");

    native_context.on_native_exception = env->GetMethodID(
            native_context.jniDelegateClass,
            on_native_exceptionMethod, "(Ljava/lang/String;)V");

    native_context.on_application_not_responding = env->GetMethodID(
            native_context.jniDelegateClass,
            on_application_not_respondingMethod, "(Ljava/lang/String;)V");

    return true;
}

void release_delegate(JNIEnv *env) {
    jni::native_context_t &native_context = jni::get_native_context();

    // release objects allocated during binding
    env->DeleteGlobalRef(native_context.jniDelegateClass);
    env->DeleteGlobalRef(native_context.jniDelegateObject);

    native_context.jniDelegateClass = nullptr;
    native_context.jniDelegateObject = nullptr;
    native_context.on_native_crash = nullptr;
    native_context.on_native_exception = nullptr;
    native_context.on_application_not_responding = nullptr;
}

/**
 * Worker thread for crash transfer
 *
 * https://developer.android.com/training/articles/perf-jni#general-tips
 * "Avoid asynchronous communication between code written in a managed
 * programming language and code written in C++ when possible"
 *
 * @param arg global ref to passed jstring instance, should be freed on completion.
 * @return
 */
void *on_native_crashThread(void *arg) {
    jni::native_context_t &native_context = jni::get_native_context();

    JNIEnv *env;
    if (native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jint res = native_context.jvm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            _LOGE("AttachCurrentThread: errorCode = %d", res);
            return NULL;
        }
    }

    if (!native_context.on_native_crash) {
        _LOGE("Failed to retrieve on_native_crash() methodID @ line %d", __LINE__);
        return NULL;
    }

    // invoke the method passing the backtrace as a string
    jstring jbacktrace = static_cast<jstring>(arg);
    env->CallVoidMethod(native_context.jniDelegateObject,
                        native_context.on_native_crash,
                        jbacktrace);

    env->DeleteGlobalRef(jbacktrace);

    native_context.jvm->DetachCurrentThread();

    return NULL;
}

/**
 * Serialize a flattened crash report and
 * pass to delegate in agent
 *
 * @param backtrace
 */
void on_native_crash(const char *backtrace) {
    jni::native_context_t &native_context = jni::get_native_context();

    JNIEnv *env;
    if (native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jint res = native_context.jvm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            _LOGE("AttachCurrentThread: errorCode = %d", res);
            return;
        }
    }

    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    // jobject byteBuffer = env->NewDirectByteBuffer((void *) backtrace, strlen(backtrace));

    jstring jBacktrace = env->NewStringUTF(backtrace);
    jobject gRef = env->NewGlobalRef(jBacktrace);

    pthread_t threadInfo = {};
    pthread_create(&threadInfo, &threadAttr, on_native_crashThread, gRef);

    pthread_attr_destroy(&threadAttr);
}

/**
 * Serialize a flattened handled exception report and
 * pass to delegate in agent
 *
 * @param backtrace
 */
void on_native_exception(const char *backtrace) {
    jni::native_context_t &native_context = jni::get_native_context();

    // FIXME Thread the delegate call

    JNIEnv *env;
    if (native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jint res = native_context.jvm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            _LOGE("AttachCurrentThread: errorCode = %d", res);
            return;
        }
    }

    if (!native_context.on_native_exception) {
        _LOGE("Failed to retrieve on_native_crash() methodID @ line %d", __LINE__);
        return;
    }

    // invoke the method passing the backtrace as a string
    jstring jbacktrace = env->NewStringUTF(backtrace);
    env->CallVoidMethod(native_context.jniDelegateObject,
                        native_context.on_native_exception,
                        jbacktrace);

    env->DeleteLocalRef(jbacktrace);
}

/**
 * Serialize a flattened ANR report and
 * pass to delegate in agent
 *
 * @param backtrace
 */
void on_application_not_responding(const char *backtrace) {
    jni::native_context_t &native_context = jni::get_native_context();

    // FIXME Thread the delegate call

    JNIEnv *env;
    if (native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jint res = native_context.jvm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            _LOGE("AttachCurrentThread: errorCode = %d", res);
            return;
        }
    }

    if (!native_context.on_application_not_responding) {
        _LOGE("Failed to retrieve on_native_crash() methodID @ line %d", __LINE__);
        return;
    }

    // invoke the method passing the backtrace as a string
    jstring jbacktrace = env->NewStringUTF(backtrace);
    env->CallVoidMethod(native_context.jniDelegateObject,
                        native_context.on_application_not_responding,
                        jbacktrace);

    env->DeleteLocalRef(jbacktrace);
}
