/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import org.json.JSONArray
import org.json.JSONObject

/**
 * StackTraceElement is final, so cannot be derived. We'll create a delgate class
 * for use in crash instances.
 *
 * Per {@link StackTraceElement#isNativeMethod()}, line number -2 indicates this frame is native
 */
class NativeStackFrame(
    className: String = "<native>",
    methodName: String = "<unknown>",
    fileName: String = "<unknown>",
    lineNumber: Int = -2
) {

    var delegate: StackTraceElement =
        StackTraceElement(className, methodName, fileName, lineNumber)

    fun asStackTraceElement(): StackTraceElement {
        return delegate
    }

    fun fromJson(frame: JSONObject): NativeStackFrame {
        try {
            delegate = StackTraceElement(
                "0x" + frame.optLong("address", frame.optLong("so_base", -1L)).toString(16),
                frame.optString("sym_name", "???"), // "0x" + frame.optLong("sym_addr", -1L).toString(16)),
                frame.optString("so_path", "0x" + frame.optLong("so_base", -1L).toString(16)),
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
                            val stackFrame = NativeStackFrame().fromJson(frame)
                            stackFrames.add(
                                frame.optInt("index", stackFrames.size),
                                stackFrame.asStackTraceElement()
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
                            val stackFrame = NativeStackFrame().fromJson(frame)
                            stackFrames.add(frame.optInt("index", stackFrames.size), stackFrame)
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