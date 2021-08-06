/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <agent-ndk.h>

#include "signal-handler.h"
#include "anr-handler.h"

const char *get_arch() {
#if defined(__arm__)
    return "armabi-v7a";
#elif defined(__aarch64__)
    return "arm64-v8a";
#elif defined(__i386__)
    return "x86";
#elif defined(__x86_64__)
    return "x86_64";
#else
#error Unknown architecture!
#endif
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_initialize(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;

    if (signal_handler_initialize()) {
        _LOGI("%s signal handler installed: pid(%d) ppid(%d) tid(%d)",
              get_arch(), getpid(), getppid(), gettid());

        if (!anr_handler_initialize()) {
            _LOGE("ANR detection not available");
        }

        return true;
    }

    _LOGE("Error: %s", strerror(errno));

    return false;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_shutdown(JNIEnv *env, jobject thiz, jboolean hard_kill) {
    (void) env;
    (void) thiz;
    (void) hard_kill;
    signal_handler_shutdown();
    anr_handler_shutdown();
    return nullptr;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_dumpstack(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;
    char str[0x200] = "TODO: implement dumpstack()";

    // TODO: implement dumpstack()

    return env->NewStringUTF(str);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_crashNow(JNIEnv *env, jobject thiz, jstring cause) {
    (void) env;
    (void) thiz;
    (void) cause;
    // TODO emit the cause in the crash report
    env->FindClass(NULL);
    kill(0, SIGKILL);
    return NULL;
}


#ifdef TBA
static bool exception_check_and_clear(JNIEnv *env) {
    if (env != nullptr) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return true;
        }
    }
    return false;
}

static jclass safe_find_class(JNIEnv *env, const char *class_name) {
    if (env != nullptr) {
        if (class_name != NULL) {
            jclass clz = env->FindClass(class_name);
            exception_check_and_clear(env);
            return clz;
        }
    }
    return nullptr;
}

static jmethodID safe_get_method_id(JNIEnv *env, jclass clz, const char *method_name, const char *method_sig) {
    if (env != nullptr) {
        if (clz != nullptr) {
            if (method_name != nullptr) {
                if (method_sig != nullptr) {
                    jmethodID methodId = env->GetMethodID(clz, method_name, method_sig);
                    exception_check_and_clear(env);
                    return methodId;
                }
            }
        }
        return nullptr;
    }
}
#endif  // TBA
