/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import java.io.PrintStream
import java.io.PrintWriter

class NativeException() : Exception("Native") {

    var nativeStackTrace: NativeStackTrace? = null

    constructor(stackTraceAsJson: String) : this() {
        nativeStackTrace = NativeStackTrace(stackTraceAsJson)
        nativeStackTrace?.stackFrames?.apply {
            stackTrace = toTypedArray()
        }
    }

    override val message: String?
        get() = nativeStackTrace?.exceptionMessage

    /**
     * The backtrace will be comprised of the native crash stack (minus the
     * top two reporting methods), with any managed frame woven in
     */
    override fun getStackTrace(): Array<StackTraceElement> {
        nativeStackTrace?.stackFrames?.apply {
            return toTypedArray()
        }
        return mutableListOf<StackTraceElement>().toTypedArray()
    }

    override fun printStackTrace() {
        super.printStackTrace()
    }

    override fun printStackTrace(s: PrintStream?) {
        super.printStackTrace(s)
    }

    override fun printStackTrace(s: PrintWriter?) {
        super.printStackTrace(s)
    }
}