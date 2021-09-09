/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <android/log.h>
#include <android/api-level.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#include <agent-ndk.h>
#include "signal-handler.h"
#include "anr-handler.h"
#include "terminate-handler.h"
#include "unwinder.h"
#include "jni/native-context.h"
#include "serializer.h"


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
Java_com_newrelic_agent_android_ndk_AgentNDK_start(__unused JNIEnv *env, __unused jobject thiz) {
    _LOGD("Starting NewRelic native reporting: %s", "FIXME");

    if (!signal_handler_initialize()) {
        _LOGE("Error: Failed to initialize signal handlers!");
    }
    _LOGD("%s sig handlers installed:", get_arch());

    if (!anr_handler_initialize()) {
        _LOGE("Error: Failed to initialize ANR detection!");
    }
    _LOGD("ANR signal handlers installed");

    if (!terminate_handler_initialize()) {
        _LOGE("Error: Failed to initialize exception handlers!");
    }
    _LOGD("Exception handlers installed");

    _LOGD("pid: %d ppid: %d tid: %d", getpid(), getppid(), gettid());

    return true;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_stop(__unused JNIEnv *env, __unused jobject thiz, __unused jboolean hard_kill) {

    signal_handler_shutdown();
    anr_handler_shutdown();
    terminate_handler_shutdown();

    return nullptr;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_dumpstack(__unused JNIEnv *env, __unused jobject thiz) {
    char buffer[BACKTRACE_SZ_MAX];

    if (unwind_backtrace(buffer, sizeof(buffer), nullptr, nullptr)) {
        serializer::from_crash(buffer, sizeof(buffer));
        return env->NewStringUTF(buffer);      // FIXME leak
    }

    return nullptr;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_crashNow(__unused JNIEnv *env, __unused jobject thiz, __unused jstring cause) {
    syscall(SYS_tgkill, getpid(), gettid(), SIGQUIT);

    return nullptr;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_isRootedDevice(__unused JNIEnv *env, __unused jobject thiz) {

    return false;
}
