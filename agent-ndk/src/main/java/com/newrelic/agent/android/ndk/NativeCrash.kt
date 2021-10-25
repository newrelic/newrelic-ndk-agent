/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import com.google.gson.JsonObject
import com.newrelic.agent.android.analytics.AnalyticsAttribute
import com.newrelic.agent.android.analytics.AnalyticsEvent
import com.newrelic.agent.android.crash.Crash
import com.newrelic.agent.android.harvest.crash.ThreadInfo
import com.newrelic.agent.android.util.SafeJsonPrimitive
import java.util.*

class NativeCrash(val nativeException: NativeException?,
                  sessionAttributes: Set<AnalyticsAttribute?>? = HashSet<AnalyticsAttribute>(),
                  events: Collection<AnalyticsEvent?>? = HashSet<AnalyticsEvent>())
        : Crash(nativeException, sessionAttributes, events, true) {

    constructor(crashAsJson: String) : this(NativeException(crashAsJson)) {

    }

    override fun toJsonString(): String {
        return super.toJsonString()
    }

    override fun asJsonObject(): JsonObject {
        val data = super.asJsonObject()
        nativeException?.stackTraceAsJson?.let {
            data.add("native", SafeJsonPrimitive.factory(it))
        }
        return data
    }

    fun extractNativeThreads(nativeException: NativeException): List<ThreadInfo> {
        nativeException.nativeStackTrace?.apply {
            return this.threads
        }

        return mutableListOf()
    }

    public override fun extractThreads(throwable: Throwable?): List<ThreadInfo> {
        if (throwable is NativeException) {
            return extractNativeThreads(throwable)
        }

        return super.extractThreads(throwable)
    }

    override fun getAppToken(): String {
        return super.getAppToken()
    }

    override fun toString(): String {
        return super.toString()
    }

    companion object {
        val CRASH_DELIMITER = "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***"

        /**
         * Build fingerprint:
         * 'Android/aosp_flounder/flounder:5.1.51/AOSP/enh08201009:eng/test-keys'
         * The fingerprint lets you identify exactly which build the crash occurred on. This is
         * exactly the same as the ro.build.fingerprint system property.
         **/
        val BUILD_FINGERPRINT = java.lang.System.getProperty("ro.build.fingerprint", "FIXME")

        /**
         * Revision refers to the hardware rather than the software. This is usually unused but can
         * be useful to help you automatically ignore bugs known to be caused by bad hardware.
         * This is exactly the same as the ro.revision system property.
         */
        val REVISION = java.lang.System.getProperty("ro.revision", "-1")
    }
}
