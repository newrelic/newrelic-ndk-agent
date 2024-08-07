/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.harvest.crash.ThreadInfo
import org.json.JSONArray
import org.json.JSONObject

class NativeThreadInfo(nativeException: NativeException?) : ThreadInfo(nativeException) {

    constructor() : this(NativeException())

    constructor(threadInfoAsJson: String?) : this() {
        fromJson(threadInfoAsJson)
    }

    fun fromJsonObject(jsonObject: JSONObject?): NativeThreadInfo {
        jsonObject?.apply {
            try {
                crashed = jsonObject.optBoolean("crashed", false)
                state = jsonObject.optString("state", "")
                threadId = jsonObject.optLong("threadNumber", 0)
                threadName = jsonObject.optString("threadId", "")
                threadPriority = jsonObject.optInt("priority", -1)
                stackTrace = stackTraceFromJson(jsonObject.optJSONArray("stack"))

            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
        return this
    }

    fun fromJson(threadInfoAsJsonStr: String?): NativeThreadInfo {
        return fromJsonObject(threadInfoAsJsonStr?.let { JSONObject(it) })
    }

    private fun stackTraceFromJson(allFrames: JSONArray?): Array<StackTraceElement?> {
        var stack = arrayOfNulls<StackTraceElement>(0)

        try {
            allFrames?.apply {
                val ukn = "<unknown>"
                stack = arrayOfNulls(allFrames.length())
                for (i in 0 until length()) {
                    if (!isNull(i)) {
                        try {
                            val frame = get(i) as JSONObject
                            val nativeFrame = NativeStackFrame().fromJson(frame)
                            stack[i] = nativeFrame.asStackTraceElement()
                        } catch (e: Exception) {
                            stack[i] = StackTraceElement(ukn, ukn, ukn, -2)
                        }
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return stack
    }

    fun isCrashingThread(): Boolean {
        return crashed
    }

    fun getStackTrace(): Array<StackTraceElement?>? {
        return stackTrace
    }

    companion object {
        fun allThreads(allThreads: JSONArray?): MutableList<NativeThreadInfo> {
            val threads = mutableListOf<NativeThreadInfo>()

            allThreads?.apply {
                for (i in 0 until length()) {
                    if (!isNull(i)) {
                        val threadInfo = NativeThreadInfo(get(i).toString())
                        threads.add(threadInfo)
                    }
                }
            }

            return threads
        }
    }

}