/*
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import org.json.JSONObject

class NativeStackTrace(stackTraceAsString: String? = null) {
    var crashedThread: NativeThreadInfo? = null
    var threads: MutableList<NativeThreadInfo> = mutableListOf()
    var exceptionMessage: String? = "Native exception"

    init {
        stackTraceAsString?.let {
            transformNativeStackTrace(it)
        }
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
                                            "at 0x${getLong("faultAddress").toString(16)}"
                            }
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
                                crashedThread = this
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
