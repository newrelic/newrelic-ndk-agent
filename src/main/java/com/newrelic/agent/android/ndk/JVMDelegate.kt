/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

package com.newrelic.agent.android.ndk

import java.util.*

class JVMDelegate() {

    var listener: AgentNDKListener? = AgentNDK.getInstance().managedContext?.nativeReportListener

    /**
     * A native crash has been detected and forwarded to this method
     */
    fun onNativeCrash(crashAsString: String?) {
        AgentNDK.log.debug("onNativeCrash: $crashAsString")
        listener?.run {
            onNativeCrash(crashAsString)
        }
    }

    /**
     * A native runtime exception has been detected and forwarded to this method
     */
    fun onNativeException(exceptionAsString: String?) {
        AgentNDK.log.debug("onNativeException: $exceptionAsString")
        listener?.run {
            onNativeException(exceptionAsString)
        }
    }

    /**
     * ANR condition has been detected and forwarded to this method
     */
    fun onApplicationNotResponding(anrAsString: String?) {
        AgentNDK.log.debug("onApplicationNotResponding: $anrAsString")
        listener?.run {
            onApplicationNotResponding(anrAsString)
        }
    }

    companion object {
        var delegateMethods: HashMap<String?, String?> = object : HashMap<String?, String?>() {
            init {
                put("onNativeCrash", "(Ljava/lang/String;)V")
                put("onNativeException", "(Ljava/lang/String;)V")
                put("onApplicationNotRespondingMethod", "(Ljava/lang/String;)V")
            }
        }

        fun getSignature(methodName: String): String {
            return delegateMethods[methodName] ?: "()V"
        }
    }
}