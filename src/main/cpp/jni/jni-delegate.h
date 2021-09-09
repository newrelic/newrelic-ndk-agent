/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _AGENT_NDK_JNI_DELEGATE_H
#define _AGENT_NDK_JNI_DELEGATE_H

/**
 * Bind C methods to Java adapters in JNIDelegate.
 * Delegates will minimize the service time,
 * making the delegate call on worker threads.
 *
 * @param pEnv JNI environment
 * @return true if all goes well
 */
bool bind_delegate(JNIEnv *pEnv, jni::native_context_t &native_context);

/**
 * Release and reset all resources alloc'd in bind_delegate()
 */
void release_delegate(JNIEnv *pEnv);

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

#endif // _AGENT_NDK_JNI_DELEGATE_H
