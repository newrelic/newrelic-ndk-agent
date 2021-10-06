/*
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

class NativeException() : Exception("NativeException") {

    var nativeStackTrace: NativeStackTrace = NativeStackTrace(this)

    constructor(stackTraceAsJson: String) : this() {
        nativeStackTrace = NativeStackTrace(stackTraceAsJson)
    }

    override val message: String?
        get() = nativeStackTrace.exceptionMessage

    /**
     * The backtrace will be comprised of the native crash stack (minus the
     * top two reporting methods), with any managed frame woven in
     */
    override fun getStackTrace(): Array<StackTraceElement> {
        return nativeStackTrace.stackFrames.toTypedArray()
    }
}