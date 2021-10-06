/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import org.json.JSONArray
import org.json.JSONObject

/**
 * StackTraceElement is final, so cannot be derived. We'll have to create a delgate class
 * for use in crash instances.
 *
 * Per {@link StackTraceElement#isNativeMethod()}, line number -2 indicates this frame is native
 */
class NativeStackFrame(
    className: String = "",
    methodName: String = "<method name missing>",
    fileName: String = "<filename missing>",
    lineNumber: Int = -2 ) {

    var delegate: StackTraceElement =
        StackTraceElement(className, methodName, fileName, lineNumber)

    constructor(stackTraceElement: StackTraceElement) : this() {
        this.delegate = stackTraceElement
    }

    fun asStackTraceElement(): StackTraceElement {
        return delegate
    }

    fun fromJson(frame: JSONObject): NativeStackFrame {
        try {
            delegate = StackTraceElement(
                frame.optString("className", ""),
                frame.optString("methodName", ""),
                frame.optString("fileName", ""),
                frame.optInt("lineNumber", -2)
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return this
    }

    fun fromJson(stackFrameAsStr: String?): NativeStackFrame {
        return fromJson(JSONObject(stackFrameAsStr))
    }

    companion object {
        fun allFrames(allFrames: JSONArray?): MutableList<StackTraceElement> {
            var stackFrames: MutableList<StackTraceElement> = mutableListOf<StackTraceElement>()

            allFrames?.apply {
                for (i in 0 until length()) {
                    if (!isNull(i)) {
                        try {
                            val frame = get(i) as JSONObject
                            stackFrames.add(
                                NativeStackFrame().fromJson(frame).asStackTraceElement()
                            )
                        } catch (e: Exception) {
                            stackFrames.add(
                                NativeStackFrame(get(i).toString()).asStackTraceElement()
                            )
                        }
                    }
                }
            }

            return stackFrames
        }

        fun allNativeFrames(allFrames: JSONArray?): MutableList<NativeStackFrame> {
            var stackFrames: MutableList<NativeStackFrame> = mutableListOf<NativeStackFrame>()

            allFrames?.apply {
                for (i in 0 until length()) {
                    if (!isNull(i)) {
                        try {
                            val frame = get(i) as JSONObject
                            stackFrames.add(
                                NativeStackFrame().fromJson(frame)
                            )
                        } catch (e: Exception) {
                            stackFrames.add(
                                NativeStackFrame(get(i).toString())
                            )
                        }
                    }
                }
            }

            return stackFrames
        }
    }
}