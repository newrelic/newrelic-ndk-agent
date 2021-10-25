/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

package com.newrelic.agent.android.ndk

import java.util.*

class JVMDelegate() {

    /**
     * A native crash has been detected and forwarded to this method
     */
    fun onNativeCrash(crashAsString: String?) {
        AgentNDK.log.debug("onNativeCrash: $crashAsString")
        AgentNDK.getInstance().managedContext?.nativeReportListener?.run {
            onNativeCrash(crashAsString)
        }
    }

    /**
     * A native runtime exception has been detected and forwarded to this method
     */
    fun onNativeException(exceptionAsString: String?) {
        AgentNDK.log.debug("onNativeException: $exceptionAsString")
        AgentNDK.getInstance().managedContext?.nativeReportListener?.run {
            onNativeException(exceptionAsString)
        }
    }

    /**
     * ANR condition has been detected and forwarded to this method
     */
    fun onApplicationNotResponding(anrAsString: String?) {
        AgentNDK.log.debug("onApplicationNotResponding: $anrAsString")
        AgentNDK.getInstance().managedContext?.nativeReportListener?.run {
            onApplicationNotResponding(anrAsString)
        }
    }
}