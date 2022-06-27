/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <inttypes.h>
#include <android/log.h>
#include <unistd.h>
#include <pthread.h>

#include <agent-ndk.h>
#include "native-context.h"
#include "jni.h"
#include "jni-delegate.h"
#include "procfs.h"

/**
 * The JNIEnv is used for thread-local storage. For this reason, you cannot share a
 * JNIEnv between threads. Until a thread is attached, it has no JNIEnv, and cannot
 * make JNI calls. Calling AttachCurrentThread() on an already-attached thread is a no-op.
 * Threads attached through JNI must call DetachCurrentThread() before they exit.
 *
 * Attaching to the VM
 * The JNI interface pointer (JNIEnv) is valid only in the current thread. Should another thread
 * need to access the Java VM, it must first call AttachCurrentThread() to attach itself to the
 * VM and obtain a JNI interface pointer. Once attached to the VM, a native thread works just like
 * an ordinary Java thread running inside a native method. The native thread remains attached to
 * the VM until it calls DetachCurrentThread() to detach itself.
 *
 * The attached thread should have enough stack space to perform a reasonable amount of work.
 * The allocation of stack space per thread is operating system-specific. For example, using pthreads,
 * the stack size can be specified in the pthread_attr_t argument to pthread_create.
 *
 * Detaching from the VM
 * A native thread attached to the VM must call DetachCurrentThread() to detach itself before exiting.
 * A thread cannot detach itself if there are Java methods on the call stack.
 */

/**
 *  If performance is important, it's useful to query certain values once and cache
 *  the results in native code. Because there is a limit of one JavaVM per process,
 *  it's reasonable to store this data in a static local structure.
 *
 *  Class references, field IDs, and method IDs are guaranteed valid until the class is unloaded.
 *  However, jclass is a class reference, but must be protected with a call to NewGlobalRef
 *
 * onLoad initialization:
 *  * Cache the JavaVM instance
 *  * Find class ID for delegate class(es)
 *  * Create an instance of delegate class(es)
 *  * Create a global reference to delegates to access from native threads
 *
 * Note:
 *  All allocated resources are never released by application
 *  The system will free all global refs when the process is terminated, and
 *  the pairing function JNI_OnUnload() is usually not called.
 *
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void) reserved;

    std::string cstr;
    jni::native_context_t &native_context = jni::get_native_context();

    native_context.initialized = false;
    native_context.jvm = vm;

    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        _LOGE("JNI version 1.6 not supported");
        return JNI_ERR;     // JNI version not supported.
    }

    native_context.initialized = jni::bind_delegate(env, native_context);

    if (!native_context.initialized) {
        _LOGE("Could not bind to JVM delegates. Reports will cached until the next app launch.");
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    (void) reserved;

    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        jni::release_native_context(env, jni::get_native_context());
    }
}


namespace jni {

    static void env_detach(JavaVM *jvm, void *env) {
        if (jvm != NULL && env != NULL) {
            jvm->DetachCurrentThread();
        }
    }

    bool env_check_and_clear_ex(JNIEnv *env) {
        if (env != nullptr) {
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                _LOGD("env_check_and_clear_ex: JNIEnv exception cleared");
                return true;
            }
        } else {
            _LOGE("env_check_and_clear_ex: JNIEnv is null");
        }
        return false;
    }

    jclass env_find_class(JNIEnv *env, const char *class_name) {
        if (env != nullptr) {
            if (class_name != nullptr) {
                jclass result = env->FindClass(class_name);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_find_class: class name is null");
            }
        } else {
            _LOGE("env_find_class: JNIEnv is null");
        }
        return nullptr;
    }

    jclass env_get_object_class(JNIEnv *env, jobject _jobject) {
        if (env != nullptr) {
            if (_jobject != nullptr) {
                jclass result = env->GetObjectClass(_jobject);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_get_object_class: class object is null");
            }
        } else {
            _LOGE("env_get_object_class: JNIEnv is null");
        }
        return nullptr;
    }

    jfieldID env_get_fieldid(JNIEnv *env, jclass _jclass,
                               const char *field_name, const char *field_sig) {
        if (env != nullptr) {
            if (_jclass != nullptr && field_name != nullptr && field_sig != nullptr) {
                jfieldID fieldId = env->GetFieldID(_jclass, field_name, field_sig);
                env_check_and_clear_ex(env);
                return fieldId;
            } else {
                _LOGE("env_get_fieldid: class, method or signature name is null");
            }
        } else {
            _LOGE("env_get_fieldid: JNIEnv is null");
        }
        return nullptr;
    }

    jmethodID env_get_methodid(JNIEnv *env, jclass _jclass,
                               const char *method_name, const char *method_sig) {
        if (env != nullptr) {
            if (_jclass != nullptr && method_name != nullptr && method_sig != nullptr) {
                jmethodID methodId = env->GetMethodID(_jclass, method_name, method_sig);
                env_check_and_clear_ex(env);
                return methodId;
            } else {
                _LOGE("env_get_methodid: class, method or signature name is null");
            }
        } else {
            _LOGE("env_get_methodid: JNIEnv is null");
        }
        return nullptr;
    }

    void env_call_void_method(JNIEnv *env, jobject _jobject, jmethodID _jmethodID, ...) {
        if (env != nullptr) {
            if (_jobject != nullptr && _jmethodID != nullptr) {
                va_list args;
                va_start(args, _jmethodID);
                env->CallVoidMethodV(_jobject, _jmethodID, args);
                env_check_and_clear_ex(env);
                va_end(args);
            } else {
                _LOGE("env_call_void_method: class or method ID is null");
            }
        } else {
            _LOGE("env_call_void_method: JNIEnv is null");
        }
    }

    jobject env_call_object_method(JNIEnv *env, jobject _jobject, jmethodID _jmethodID, ...) {
        if (env != nullptr) {
            if (_jobject != nullptr && _jmethodID != nullptr) {
                va_list args;
                va_start(args, _jmethodID);
                jobject result = env->CallObjectMethod(_jobject, _jmethodID, args);
                env_check_and_clear_ex(env);
                va_end(args);
                return result;
            } else {
                _LOGE("env_call_object_method: class or method ID is null");
            }
        } else {
            _LOGE("env_call_object_method: JNIEnv is null");
        }
        return nullptr;
    }

    jboolean env_call_bool_method(JNIEnv *env, jobject _jobject, jmethodID _jmethodID, ...) {
        if (env != nullptr) {
            if (_jobject != nullptr && _jmethodID != nullptr) {
                va_list args;
                va_start(args, _jmethodID);
                jboolean result = env->CallBooleanMethodV(_jobject, _jmethodID, args);
                env_check_and_clear_ex(env);
                va_end(args);
                return result;
            } else {
                _LOGE("env_call_bool_method: class or method ID is null");
            }
        } else {
            _LOGE("env_call_bool_method: JNIEnv is null");
        }
        return false;
    }

    jobject env_new_object(JNIEnv *env, jclass _jobject, jmethodID _jmethodID, ...) {
        if (env != nullptr) {
            if (_jobject != nullptr && _jmethodID != nullptr) {
                va_list args;
                va_start(args, _jmethodID);
                jobject result = env->NewObjectV(_jobject, _jmethodID, args);
                env_check_and_clear_ex(env);
                va_end(args);
                return result;
            } else {
                _LOGE("env_new_object: class name or method ID is null");
            }
        } else {
            _LOGE("env_new_object: JNIEnv is null");
        }
        return nullptr;
    }

    jobject env_new_global_ref(JNIEnv *env, jobject _jobject) {
        if (env != nullptr) {
            if (_jobject != nullptr) {
                jobject result = env->NewGlobalRef(_jobject);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_new_global_ref: passed jobject is null");
            }
        } else {
            _LOGE("env_new_global_ref: JNIEnv is null");
        }
        return nullptr;
    }

    void env_delete_global_ref(JNIEnv *env, jobject _jobject) {
        if (env != nullptr) {
            if (_jobject != nullptr) {
                env->DeleteGlobalRef(_jobject);
                env_check_and_clear_ex(env);
            } else {
                _LOGE("env_delete_global_ref: passed jobject is null");
            }
        } else {
            _LOGE("env_delete_global_ref: JNIEnv is null");
        }
    }

    jstring env_new_string_utf(JNIEnv *env, const char *cstr) {
        if (env != nullptr) {
            if (cstr != nullptr) {
                jstring result = env->NewStringUTF(cstr);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_new_string_utf: passed string is null");
            }
        } else {
            _LOGE("env_new_string_utf: JNIEnv is null");
        }
        return nullptr;
    }

    jobject env_get_object_field(JNIEnv *env, jobject _jobject, jfieldID _jfieldID) {
        if (env != nullptr) {
            if (_jobject != nullptr && _jfieldID != nullptr) {
                jobject result = env->GetObjectField(_jobject, _jfieldID);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_get_object_field: class or field ID is null");
            }
        } else {
            _LOGE("env_get_object_field: JNIEnv is null");
        }
        return nullptr;
    }

    const char* env_get_string_UTF_chars(JNIEnv *env, jstring _jstring) {
        if (env != nullptr) {
            if (_jstring != nullptr) {
                jboolean isCopy;
                const char* result = env->GetStringUTFChars(_jstring, &isCopy);
                env_check_and_clear_ex(env);
                return result;
            } else {
                _LOGE("env_get_string_UTF_chars: jstring is null");
            }
        } else {
            _LOGE("env_get_string_UTF_chars: JNIEnv is null");
        }
        return nullptr;
    }
}   // namespace jni


