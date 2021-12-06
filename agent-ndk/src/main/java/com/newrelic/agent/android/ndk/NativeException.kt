/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

class NativeException(val stackTraceAsJson: String? = null) : Exception("Native") {

    var nativeStackTrace: NativeStackTrace? = null

    init {
        stackTraceAsJson?.let {
            nativeStackTrace = NativeStackTrace(stackTraceAsJson)
            stackTrace = getStackTrace()
        }
    }

    override val message: String?
        get() = nativeStackTrace?.exceptionMessage

    /**
     * The backtrace will be comprised of the native crash stack
     * with any managed frames woven in
     */
    override fun getStackTrace(): Array<StackTraceElement> {
        val stacks = mutableListOf<StackTraceElement>()
        nativeStackTrace?.crashedThread?.getStackTrace()?.forEach {
            it?.let {
                stacks.add(it)
            }
        }
        return stacks.toTypedArray()
    }
}