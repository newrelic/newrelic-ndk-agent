/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples

import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.widget.TextView
import com.newrelic.agent.android.ndk.AgentNDK
import com.newrelic.agent.android.ndk.AgentNDKListener
import com.newrelic.agent.android.ndk.NativeCrash
import com.newrelic.agent.android.ndk.NativeException
import java.util.*
import java.util.concurrent.TimeUnit

class NewRelicAgent(var activity: Activity) : AgentNDKListener {

    fun onCreate() {
        // Change these to suite your testing
        val buildId = UUID.randomUUID().toString()
        val sessionId = UUID.randomUUID().toString()
        val reportTTL = TimeUnit.SECONDS.convert(3, TimeUnit.DAYS)

        AgentNDK.Builder(activity.applicationContext)
            .withReportListener(this)
            .withBuildId(buildId)
            .withSessionId(sessionId)
            .withExpiration(reportTTL)
            .build()
    }

    fun onStart() {
        AgentNDK.getInstance().start()
    }

    fun onStop() {
        AgentNDK.getInstance().stop()
    }

    fun onDestroy() {
        onStop()
    }

    override fun onNativeCrash(crashAsString: String?): Boolean {
        val exception = NativeException(crashAsString)
        val crash = NativeCrash(exception)
        val crashStr = crash.toJsonString()

        activity.findViewById<TextView>(R.id.text).text = crashStr
        postToClipboard(crashStr)

        return true
    }

    override fun onNativeException(exceptionAsString: String?): Boolean {
        val exception = NativeException(exceptionAsString!!)
        val exStr = exception.stackTraceToString()

        activity.findViewById<TextView>(R.id.text).text = exStr
        postToClipboard(exStr)

        return true
    }

    override fun onApplicationNotResponding(anrAsString: String?): Boolean {
        val exception = NativeException(anrAsString!!)
        val exStr = exception.stackTraceToString()

        activity.findViewById<TextView>(R.id.text).text = exStr
        postToClipboard(exStr)

        return true
    }

    private fun postToClipboard(str: String) {
        val clipboard: ClipboardManager =
            activity.getSystemService(Activity.CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText("backtrace", str)
        clipboard.setPrimaryClip(clip)
    }


}