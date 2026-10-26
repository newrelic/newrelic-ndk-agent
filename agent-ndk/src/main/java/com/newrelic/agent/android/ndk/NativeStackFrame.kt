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

    /** Absolute runtime address of the frame (varies per run due to ASLR) */
    var address: Long = 0L

    /** Address relative to the base of the containing shared object, as needed by symbolication */
    var pc: Long = 0L

    /** Base address of the containing shared object */
    var soBase: Long = 0L

    /** Offset of the frame from the start of the nearest symbol */
    var symAddrOffset: Long = 0L

    fun asStackTraceElement(): StackTraceElement {
        return delegate
    }

    fun fromJson(frame: JSONObject): NativeStackFrame {
        try {
            address = frame.optLong("address", 0L)
            pc = frame.optLong("pc", 0L)
            soBase = frame.optLong("so_base", 0L)
            symAddrOffset = frame.optLong("sym_addr_offset", 0L)

            // The absolute address is randomized per process (ASLR) and useless for symbolication
            // without the so base. Prefer the module-relative pc, which is unresolved (0) only
            // when dladdr failed to locate the frame.
            val location = if (pc != 0L) pc else frame.optLong("address", frame.optLong("so_base", -1L))
            val symName = frame.optString("sym_name", "")
            delegate = StackTraceElement(
                "0x" + location.toString(16),
                if (symName.isEmpty()) "???" else "$symName+$symAddrOffset",
                frame.optString("so_path", "0x" + frame.optLong("so_base", -1L).toString(16)),
                frame.optInt("lineNumber", -2)
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return this
    }

    fun fromJson(stackFrameAsStr: String): NativeStackFrame {
        return fromJson(JSONObject(stackFrameAsStr))
    }

    companion object {
        fun allFrames(allFrames: JSONArray?): MutableList<StackTraceElement> {
            val stackFrames: MutableList<StackTraceElement> = mutableListOf()

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
            val stackFrames: MutableList<NativeStackFrame> = mutableListOf()

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