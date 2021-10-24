/*
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.harvest.crash.ThreadInfo
import org.json.JSONObject

class NativeStackTrace(val exception: Exception) {

    var crashedThreadId: Long = 0L
    var stackFrames: MutableList<StackTraceElement> = mutableListOf()
    var threads: MutableList<NativeThreadInfo> = mutableListOf()
    var exceptionMessage: String? = "Native exception"

    constructor(stackTraceAsString: String = "{}") : this(Exception("Native stacktrace")) {
        transformNativeStackTrace(stackTraceAsString)
    }

    private fun transformNativeStackTrace(stackTraceAsString: String?): NativeStackTrace {
        return transformNativeStackTrace(JSONObject(stackTraceAsString))
    }

    private fun transformNativeStackTrace(jsonObj: JSONObject): NativeStackTrace {
        try {
            jsonObj.apply {
                getJSONObject("backtrace")?.apply {
                    try {
                        getJSONObject("exception")?.apply {
                            val cause = optString("cause", "")
                            exceptionMessage = "${getString("name")}: ${cause}"
                            getJSONObject("signalInfo")?.apply {
                                exceptionMessage =
                                    "${getString("signalName")} " +
                                            "(code ${getInt("signalCode")}) ${cause} " +
                                            "at ${getString("faultAddress")}"
                            }
                        }
                    } catch (ignored: Exception) {
                        ignored.printStackTrace()
                    }

                    try {
                        getJSONArray("stackframes")?.apply {
                            stackFrames = NativeStackFrame.allFrames(this)
                        }
                    } catch (ignored: Exception) {
                        ignored.printStackTrace()
                    }

                    try {
                        getJSONArray("threads")?.apply {
                            threads = NativeThreadInfo.allThreads(this)
                            threads.find() {
                                it.isCrashingThread()
                            }?.apply {
                                crashedThreadId = threadId
                            }
                        }
                    } catch (ignored: Exception) {
                        ignored.printStackTrace()
                    }
                }
            }
        } catch (e: Exception) {
            exceptionMessage = e.message.toString()
        }

        return this
    }

}
