/*
 * Copyright (c) 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

open class NativeException(val stackTraceAsJson: String? = null) :
    RuntimeException("New Relic native exception") {

    var nativeStackTrace: NativeStackTrace? = null

    init {
        stackTrace = Thread.currentThread().stackTrace
        stackTraceAsJson?.apply {
            nativeStackTrace = NativeStackTrace(this)
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
            it?.apply {
                stacks.add(this)
            }
        }
        return stacks.toTypedArray()
    }
}