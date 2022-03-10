/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _AGENT_NDK_JNI_DELEGATE_H
#define _AGENT_NDK_JNI_DELEGATE_H

namespace jni {

    /**
     * Bind C methods to Java adapters in JavaDelegate.
     * Delegates will minimize the service time,
     * making the delegate calls on worker threads.
     *
     * @param env JNI environment
     * @return true JVM classes are bound to JNI structs
     */
    bool bind_delegate(JNIEnv *env, jni::native_context_t &native_context);

    /**
     * Release and reset all resources alloc'd in bind_delegate()
     */
    void release_delegate(JNIEnv *env, jni::native_context_t &native_context);

    /**
     * Called with a serialized crash report (WIP)
     * @param backtrace
     */
    void on_native_crash(const char *backtrace);

    /**
     * Called with a serialized handled exception (WIP)
     * @param backtrace
     */
    void on_native_exception(const char *backtrace);

    /**
     * Called with a serialized handled exception in
     * response to an ANR signal (WIP)
     * @param backtrace
     */
    void on_application_not_responding(const char *backtrace);

}   // namespace jni

#endif // _AGENT_NDK_JNI_DELEGATE_H

