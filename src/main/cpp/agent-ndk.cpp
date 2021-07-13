/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <android/log.h>
#include "signal-handlers.h"

static const char* TAG = "nr-agent-ndk";

static const char *get_arch() {
#if defined(__arm__)
    return "ARM32";
#elif defined(__aarch64__)
    return "ARM64";
#elif defined(__i386__)
    return "X86";
#elif defined(__x86_64__)
    return "X86_64";
#else
#error Unknown architecture!
#endif
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_initialize(JNIEnv *env, jobject thiz) {
    char str[0x200];

    if (signal_handler_initialize()) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,
                            "%s sig handlers installed: pid(%d) ppid(%d) tid(%d)",
                            get_arch(), getpid(), getppid(), gettid());
        return true;
    }

    __android_log_print(ANDROID_LOG_ERROR, TAG, "Error: %s", strerror(errno));

    return false;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_shutdown(JNIEnv *env, jobject thiz, jboolean hard_kill) {
    signal_handler_shutdown();
    return NULL;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_dumpstack(JNIEnv *env, jobject thiz) {
    char str[0x200];

    // TODO: implement dumpstack()

    return env->NewStringUTF(str);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_crashNow(JNIEnv *env, jobject thiz, jstring cause) {
    kill(0, SIGKILL);
    return NULL;
}

