/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples

import android.app.Activity
import android.widget.TextView
import com.newrelic.agent.android.FeatureFlag
import com.newrelic.agent.android.NewRelic
import com.newrelic.agent.android.logging.AgentLog
import com.newrelic.agent.android.logging.AgentLogManager

class NewRelicAgent(private val activity: Activity) {

    private var legacyAgent: NewRelic? = null
    private var agentLog: AgentLog? = AgentLogManager.getAgentLog()

    fun onCreate() {
        NewRelic.enableFeature(FeatureFlag.NativeReporting)
        NewRelic.disableFeature(FeatureFlag.ApplicationExitReporting)
        NewRelic.disableFeature(FeatureFlag.LogReporting)

        legacyAgent = NewRelic.withApplicationToken("<APP-TOKEN>")
            .withLogLevel(AgentLog.DEBUG)
            .withApplicationBuild("Legacy Native agent")

        activity.findViewById<TextView>(R.id.text)?.text = activity.getString(R.string.created)
        agentLog?.info("Legacy agent was created")

    }

    fun onStart() {
        legacyAgent?.start(activity.applicationContext)
        activity.findViewById<TextView>(R.id.text)?.text = activity.getString(R.string.started)
        agentLog?.info("Legacy agent has started")
    }

    fun onStop() {
        activity.findViewById<TextView>(R.id.text)?.text = activity.getString(R.string.stopped)
        agentLog?.info("Legacy agent has stopped")
    }

    fun onDestroy() {
        onStop()
        activity.findViewById<TextView>(R.id.text)?.text = activity.getString(R.string.destroyed)
        agentLog?.info("Legacy agent destroyed")
    }

}