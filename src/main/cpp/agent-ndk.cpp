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
#include <sys/ucontext.h>
#include <sys/syscall.h>

#include <agent-ndk.h>
#include "signal-handler.h"
#include "anr-handler.h"
#include "terminate-handler.h"
#include "unwinder.h"
#include "jni/jni.h"
#include "jni/native-context.h"
#include "jni/jni-delegate.h"
#include "serializer.h"
#include "procfs.h"


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

bool arch_is_32b() {
    return strstr(get_arch(), "64") == nullptr;
}

volatile bool initialized = false;

extern "C" JNIEXPORT
jboolean JNICALL Java_com_newrelic_agent_android_ndk_AgentNDK_nativeStart(JNIEnv *env,jobject thz,
                                        jobject managedContext) {
    (void) env;
    (void) thz;
    std::string cstr;

    _LOGD("Starting NewRelic native reporting: %s", AGENT_VERSION);
    _LOGD("    pid: %d ppid: %d tid: %d", getpid(), getppid(), gettid());

    jni::native_context_t &native_context = jni::set_native_context(env, managedContext);

    native_context.initialized = bind_delegate(env, native_context);
    if (!native_context.initialized) {
        _LOGE("Could not bind to JVM delegates. Reports will cached until the next app launch.");
    }

    if (!signal_handler_initialize()) {
        _LOGE("Error: Failed to initialize signal handlers!");
    }
    _LOGD("%s signal handler installed", get_arch());

    if (!anr_handler_initialize()) {
        _LOGE("Error: Failed to initialize ANR detection!");
    }
    _LOGD("ANR signal handler installed");

    if (!terminate_handler_initialize()) {
        _LOGE("Error: Failed to initialize exception handlers!");
    }
    _LOGD("Exception handler installed");

    initialized = true;

    return initialized;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_nativeStop(JNIEnv *env, jobject thiz, jboolean hard_kill) {
    (void) env;
    (void) thiz;
    (void) hard_kill;

    signal_handler_shutdown();
    anr_handler_shutdown();
    terminate_handler_shutdown();
}

extern "C"
JNIEXPORT jstring JNICALL Java_com_newrelic_agent_android_ndk_AgentNDK_dumpStack(JNIEnv * env, jobject thiz) {
    (void) env;
    (void) thiz;

    char buffer[BACKTRACE_SZ_MAX];
    siginfo_t _siginfo = {};
    ucontext_t _sa_ucontext = {};
        if (unwind_backtrace(buffer, sizeof(buffer), &_siginfo, &_sa_ucontext)) {
        return env->NewStringUTF(buffer);      // FIXME leak
    }

    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_crashNow(JNIEnv *env, jobject thiz, jstring cause) {
    (void) env;
    (void) thiz;
    (void) cause;
    syscall(SYS_tgkill, getpid(), gettid(), SIGQUIT);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_isRootedDevice(JNIEnv * env, jobject thiz) {
    (void) env;
    (void) thiz;

    // TODO

    return false;
}

extern "C"
JNIEXPORT void JNICALL Java_com_newrelic_agent_android_ndk_AgentNDK_setNativeContext(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jobject managedContext) {
    (void) thiz;
    jni::set_native_context(env, managedContext);
}

