/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

interface AgentNDKListener {
    /**
     * A native crash has been detected and forwarded to this method
     */
    fun onNativeCrash(crashAsString: String?)

    /**
     * A native runtime exception has been detected and forwarded to this method
     */
    fun onNativeException(exceptionAsString: String?)

    /**
     * ANR condition has been detected and forwarded to this method
     */
    fun onApplicationNotResponding(stackTraceAsString: String?)
}