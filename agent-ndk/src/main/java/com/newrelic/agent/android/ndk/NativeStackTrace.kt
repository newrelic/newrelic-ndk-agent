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

    /**
     * Signals raised by a process (kill, abort, ...) have a non-positive si_code. For those the
     * kernel's si_addr is not a fault address: it overlaps the sender's pid and uid.
     */
    private fun signalMessage(signalInfo: JSONObject, cause: String): String {
        val signalName = signalInfo.getString("signalName")
        val signalCode = signalInfo.getInt("signalCode")
        val message = StringBuilder("$signalName (code $signalCode)")

        // the cause is sometimes just the signal name again
        if (cause.isNotEmpty() && cause != signalName) {
            message.append(" $cause")
        }

        if (signalCode > 0) {
            message.append(" at 0x${signalInfo.getLong("faultAddress").toString(16)}")
        } else if (signalInfo.has("senderPid")) {
            message.append(
                " sent by pid ${signalInfo.getInt("senderPid")}" +
                        " (uid ${signalInfo.optInt("senderUid", -1)})"
            )
        }

        return message.toString()
    }

    private fun transformNativeStackTrace(stackTraceAsString: String): NativeStackTrace {
        return transformNativeStackTrace(JSONObject(stackTraceAsString))
    }

    private fun transformNativeStackTrace(jsonObj: JSONObject): NativeStackTrace {
        try {
            jsonObj.apply {
                getJSONObject("backtrace").apply {
                    try {
                        getJSONObject("exception").apply {
                            val cause = optString("cause", "")
                            exceptionMessage = "${getString("name")}: $cause"
                            getJSONObject("signalInfo").apply {
                                exceptionMessage = signalMessage(this, cause)
                            }
                        }
                    } catch (ignored: Exception) {
                        ignored.printStackTrace()
                    }

                    try {
                        getJSONArray("threads").apply {
                            threads = NativeThreadInfo.allThreads(this)
                            threads.find {
                                it.isCrashingThread()
                            }.apply {
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
