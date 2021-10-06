/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.harvest.crash.ThreadInfo
import org.json.JSONArray
import org.json.JSONObject

/**
 * TODO Address dependency on agent-core
 */


class NativeThreadInfo(throwable: Throwable? = NativeException()) : ThreadInfo(throwable) {

    constructor(threadInfoAsJson: String?) : this() {
        fromJson(threadInfoAsJson)
    }

    fun fromJsonObject(jsonObject: JSONObject?): NativeThreadInfo {
        jsonObject?.apply {
            try {
                crashed = jsonObject.optBoolean("crashed", false)
                state = jsonObject.optString("state", "")
                threadId = jsonObject.optLong("threadid", 0)
                threadName = jsonObject.optString("name", "")
                threadPriority = jsonObject.optInt("priority", -1)
                stackTrace = stackTraceFromJson(jsonObject.optJSONArray("stack"))

            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
        return this
    }

    fun fromJson(threadInfoAsJsonStr: String?): NativeThreadInfo {
        return fromJsonObject(JSONObject(threadInfoAsJsonStr))
    }

    fun stackTraceFromJson(allFrames: JSONArray?): Array<StackTraceElement?>? {
        var stack = arrayOfNulls<StackTraceElement>(0)

        try {
            allFrames?.apply {
                val ukn = "<unknown>"

                stack = arrayOfNulls<StackTraceElement>(allFrames.length())
                for (i in 0 until length()) {
                    if (!isNull(i)) {
                        try {
                            val frame = get(i) as JSONObject
                            val fileName = frame.optString("filename", ukn)
                            val className = frame.optString("className", ukn)
                            val methodName = frame.optString("methodName", ukn)
                            val lineNumber = frame.optInt("lineNumber", -2)

                            stack.set(
                                i,
                                StackTraceElement(className, methodName, fileName, lineNumber)
                            )
                        } catch (e: Exception) {
                            stack.set(i, StackTraceElement(ukn, ukn, ukn, -2));
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
        return crashed;
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

        fun allThreads(allThreadsAsString: String?): MutableList<NativeThreadInfo> {
            return allThreads(JSONArray(allThreadsAsString))
        }

        fun fromJson(jsonObject: JSONObject?): NativeThreadInfo {
            return NativeThreadInfo().fromJsonObject(jsonObject)
        }
    }
}