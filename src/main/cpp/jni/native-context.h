/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _AGENT_NDK_NATIVE_CONTEXT_H
#define _AGENT_NDK_NATIVE_CONTEXT_H

#include <jni.h>

namespace jni {

    typedef struct native_context {
        bool initialized;
        JavaVM *jvm;

        // the crash serializer delegate class and global references
        jclass jniDelegateClass;
        jobject jniDelegateObject;

        // delegate method IDs
        jmethodID on_native_crash;
        jmethodID on_native_exception;
        jmethodID on_application_not_responding;

    } native_context_t;

    // FIXME returns global instance
    native_context_t &get_native_context();

    void release_native_context(JNIEnv *env);

    }   // namespace jni

#endif // _AGENT_NDK_NATIVE_CONTEXT_H

