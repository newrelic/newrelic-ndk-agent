/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import com.google.gson.JsonArray
import com.google.gson.JsonObject
import com.newrelic.agent.android.analytics.AnalyticsAttribute
import com.newrelic.agent.android.analytics.AnalyticsEvent
import com.newrelic.agent.android.crash.Crash
import com.newrelic.agent.android.harvest.crash.ExceptionInfo
import com.newrelic.agent.android.harvest.crash.ThreadInfo

/**
 * TODO Address dependency on agent-core
 */

class NativeCrash : Crash {
    constructor(
        throwable: Throwable?, sessionAttributes: Set<AnalyticsAttribute?>?,
        events: Collection<AnalyticsEvent?>?, analyticsEnabled: Boolean
    ) : super(throwable, sessionAttributes, events, analyticsEnabled) {
    }

    constructor(throwable: Throwable?) : super(throwable) {}

    override fun getExceptionInfo(): ExceptionInfo {
        return super.getExceptionInfo()
    }

    override fun asJsonObject(): JsonObject {

        // data.add("threads", getThreadsAsJson());
        // data.add("native", getThreadsAsJson());

        return super.asJsonObject()
    }

    override fun getThreadsAsJson(): JsonArray {
        return super.getThreadsAsJson()
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
