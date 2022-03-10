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
        jobject jniDelegateObject;
        jmethodID onNativeCrash;
        jmethodID onNativeException;
        jmethodID onApplicationNotResponding;

        // report storage
        char reportPathAbsolute[PATH_MAX];

        // session id passed from client
        char sessionId[40];

        // build id passed from client
        char buildId[40];

    } native_context_t;

    /**
     * Returns a populated native context singleton
     * @return native_context instance
     */
    native_context_t &get_native_context();

    /**
     * Conditions the native context from a passed JVM ManagedContext instance
     * @param env
     * @param managedContext instance
     * @return
     */
    native_context_t &set_native_context(JNIEnv *env, jobject managedContext);

    /**
     * Clean up JNI mem tied to the native_context
     *
     * @param env JNI env
     * @param native_context instance
     */
    void release_native_context(JNIEnv *env, native_context_t &native_context);

}   // namespace jni

#endif // _AGENT_NDK_NATIVE_CONTEXT_H

