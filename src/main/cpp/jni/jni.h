/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_JNI_H
#define _AGENT_NDK_JNI_H

#ifdef __cplusplus
extern "C" {
#endif

namespace jni {

    bool env_check_and_clear_ex(JNIEnv *);

    jclass env_find_class(JNIEnv *, const char *);

    jmethodID env_get_methodid(JNIEnv *, jclass, const char *, const char *);

    jobject env_new_object(JNIEnv *, jclass, jmethodID, ...);

    jobject env_new_global_ref(JNIEnv *, jobject);

    void env_delete_global_ref(JNIEnv *, jobject);

    jstring env_new_string_utf(JNIEnv *, const char *);

    void env_call_void_method(JNIEnv *, jobject, jmethodID, ...);

    }   // namespace procfs

#ifdef __cplusplus
}
#endif

#endif // _AGENT_NDK_JNI_H
