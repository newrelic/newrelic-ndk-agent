/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <pthread.h>

#include <agent-ndk.h>
#include "jni.h"
#include "native-context.h"

namespace jni {

    static const char *delegate_class = "com/newrelic/agent/android/ndk/JVMDelegate";
    static const char *on_native_crash_method = "onNativeCrash";
    static const char *on_native_exception_method = "onNativeException";
    static const char *on_application_not_responding_method = "onApplicationNotResponding";
    static const char *delegate_method_sig = "(Ljava/lang/String;)V";

    bool bind_delegate(JNIEnv *env, jni::native_context_t &native_context) {
        jclass jniDelegateClass = jni::env_find_class(env, delegate_class);

        if (jniDelegateClass == nullptr) {
            _LOGE("Unable to find AgentNDK delegate class [%s]", delegate_class);
            return JNI_ERR;
        }

        jmethodID delegateCtor = jni::env_get_methodid(env, jniDelegateClass, "<init>", "()V");
        jobject delegateObj = jni::env_new_object(env, jniDelegateClass, delegateCtor);

        // cache the jni vars
        native_context.jniDelegateObject = jni::env_new_global_ref(env, delegateObj);
        if (native_context.jniDelegateObject == nullptr) {
            _LOGE("Unable to create a global ref to the AgentNDK delegate class [%s]",
                  delegate_class);
            return JNI_ERR;
        }

        // set the delegate method IDs (opaque structs that should not be cast to jobject)
        native_context.onNativeCrash = jni::env_get_methodid(env,
                                                             jniDelegateClass,
                                                             on_native_crash_method,
                                                             delegate_method_sig);

        if (!native_context.onNativeCrash) {
            _LOGE("Failed to retrieve on_native_crash() method id");
            return false;
        }

        native_context.onNativeException = jni::env_get_methodid(env,
                                                                 jniDelegateClass,
                                                                 on_native_exception_method,
                                                                 delegate_method_sig);

        if (!native_context.onNativeException) {
            _LOGE("Failed to retrieve on_native_crash() method id");
            return false;
        }

        native_context.onApplicationNotResponding = jni::env_get_methodid(env,
                                                                          jniDelegateClass,
                                                                          on_application_not_responding_method,
                                                                          delegate_method_sig);

        if (!native_context.onApplicationNotResponding) {
            _LOGE("Failed to retrieve on_native_crash() method id");
            return false;
        }

        native_context.initialized = true;

        return native_context.initialized;
    }

    void release_delegate(JNIEnv *env, jni::native_context_t &native_context) {

        if (env != nullptr) {
            // release objects allocated during binding
            jni::env_delete_global_ref(env, native_context.jniDelegateObject);

            native_context.jniDelegateObject = nullptr;
            native_context.onNativeCrash = nullptr;
            native_context.onNativeException = nullptr;
            native_context.onApplicationNotResponding = nullptr;
        }
    }

    /**
     * Worker thread for payload exchange
     *
     * @param arg global ref to passed jstring instance, should be freed on completion.
     */

    typedef struct delegate_thread_args {
        JNIEnv *env;
        jobject delegate;
        jmethodID method;
        jobject backtrace;

    } delegate_thread_args_t;

    void *delegate_worker_thread(void *args) {
        jni::native_context_t &native_context = jni::get_native_context();
        delegate_thread_args_t *delegate_args = (delegate_thread_args *) args;
        bool jni_attached = false;

        JNIEnv *env = nullptr;
        int jrc = native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
        switch (jrc) {
            case JNI_OK:
                break;
            case JNI_EDETACHED:
                jrc = native_context.jvm->AttachCurrentThread(&env, nullptr);
                if (JNI_OK != jrc) {
                    _LOGE("delegate_worker_thread: AttachCurrentThread failed: error %d: %s", jrc,
                          strerror(jrc));
                    return nullptr;
                }
                jni_attached = true;
                jni::env_check_and_clear_ex(env);
                jni_attached = true;
                break;
            default:
                _LOGE("delegate_worker_thread:: Unsupported JNI version");
                return nullptr;
        };  // switch

        // invoke the delegate method passing the backtrace as a string
        jni::env_call_void_method(env,
                                  delegate_args->delegate,
                                  delegate_args->method,
                                  delegate_args->backtrace);

        // release the memory alloc'd in calling thread
        jni::env_delete_global_ref(env, delegate_args->backtrace);
        delete delegate_args;

        // and detach from JVM
        if (jni_attached) {
            native_context.jvm->DetachCurrentThread();
        }

        return nullptr;
    }

    /**
     * Serialize a flattened report and pass to
     * delegate on a JVM-bound thread
     *
     * @param backtrace backtrace contained in a char buffer
     * @param methodId Method ID of jvm delegate
     */
    bool threaded_delegate_call(const char *backtrace, jmethodID methodId) {
        jni::native_context_t &native_context = jni::get_native_context();
        if (native_context.jvm == nullptr) {
            _LOGE("on_native_crash: jvm not supported in native_context");
            return false;
        }

        bool jni_attached = false;
        JNIEnv *env = nullptr;
        int jrc = native_context.jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
        switch (jrc) {
            case JNI_OK:
                break;
            case JNI_EDETACHED:
                jrc = native_context.jvm->AttachCurrentThread(&env, nullptr);
                if (JNI_OK != jrc) {
                    _LOGE("threaded_delegate_call: AttachCurrentThread failed: error %d: %s", jrc,
                          strerror(jrc));
                    return false;
                }
                jni_attached = true;
                break;
            default:
                _LOGE("delegate_worker_thread:: Unsupported JNI version");
                return false;
        };  // switch

        delegate_thread_args_t *delegate_thread_args = new delegate_thread_args_t();
        if (delegate_thread_args == nullptr) {
            _LOGE("thread_delegate_call: Failed to allocate delegate thread args");
            return false;
        }

        jstring jBacktrace = jni::env_new_string_utf(env, backtrace);
        jobject jBacktraceRef = jni::env_new_global_ref(env, jBacktrace);

        delegate_thread_args->env = env;
        delegate_thread_args->delegate = native_context.jniDelegateObject;
        delegate_thread_args->method = methodId;
        delegate_thread_args->backtrace = jBacktraceRef;

        // delegate_worker_thread((void*) delegate_thread_args);

        pthread_t delegate_thread = {};
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&delegate_thread, &thread_attr, delegate_worker_thread,
                           delegate_thread_args) != 0) {
            _LOGE("threaded_delegate_call: thread creation failed");

            // release the memory alloc'd above
            pthread_attr_destroy(&thread_attr);
            jni::env_delete_global_ref(env, jBacktraceRef);
            delete delegate_thread_args;

            return false;
        }

        _LOGD("threaded_delegate_call: delegate running on thread [%p]", (void *) delegate_thread);
        pthread_attr_destroy(&thread_attr);

        pthread_join(delegate_thread, nullptr);

        if (jni_attached) {
            native_context.jvm->DetachCurrentThread();
        }

        return true;
    }


    /**
     * Pass a flattened crash report to delegate in agent on a JVM-bound thread
     *
     * @param backtrace
     */
    void on_native_crash(const char *backtrace) {
        jni::native_context_t &native_context = jni::get_native_context();

        if (native_context.jvm == nullptr) {
            _LOGE("on_native_crash: jvm not supported in native_context");
            return;
        }

        threaded_delegate_call(backtrace, native_context.onNativeCrash);
    }


    /**
     * Pass a flattened native exception report to delegate in agent on a JVM-bound thread
     *
     * @param backtrace
     */
    void on_native_exception(const char *backtrace) {
        jni::native_context_t &native_context = jni::get_native_context();
        if (native_context.jvm == nullptr) {
            _LOGE("on_native_exception: no jvm available in native_context");
            return;
        }

        threaded_delegate_call(backtrace, native_context.onNativeException);
    }

    /**
     * Pass a flattened ANR report to delegate in agent on a JVM-bound thread
     *
     * @param backtrace
     */
    void on_application_not_responding(const char *backtrace) {
        jni::native_context_t &native_context = jni::get_native_context();
        if (native_context.jvm == nullptr) {
            _LOGE("on_native_crash: no jvm available in native_context");
            return;
        }
        threaded_delegate_call(backtrace, native_context.onApplicationNotResponding);
    }

}   // namespace jni
