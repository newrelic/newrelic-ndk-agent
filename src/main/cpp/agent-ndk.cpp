/**
 *
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * References:
 * https://android.googlesource.com/platform/system/core/+/refs/heads/master/debuggerd/
 *
 */

#include <jni.h>
#include <string>
#include <cerrno>

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_initialize(JNIEnv *env, jobject thiz) {
    // TODO: implement initialize()
    return false;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_AgentNDK_shutdown(JNIEnv *env, jobject thiz, jboolean hard_kill) {
    // TODO: implement shutdown()
    return NULL;
}

