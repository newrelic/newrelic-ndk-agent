/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

interface AgentNDKListener {
    /**
     * A native crash has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    fun onNativeCrash(crashAsString: String?) : Boolean

    /**
     * A native runtime exception has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    fun onNativeException(exceptionAsString: String?) : Boolean

    /**
     * ANR condition has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    fun onApplicationNotResponding(anrAsString: String?) : Boolean
}