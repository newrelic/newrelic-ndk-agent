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
import com.newrelic.agent.android.ndk.AgentNDK

class NewRelicAgent(val activity: Activity) {

    private var legacyAgent: NewRelic? = null
    private var agentLog: AgentLog? = AgentLogManager.getAgentLog()

    fun onCreate() {
        NewRelic.enableFeature(FeatureFlag.NativeReporting)

        legacyAgent = NewRelic.withApplicationToken("<app-token>")
            .withLogLevel(AgentLog.DEBUG)
            .withApplicationBuild("Legacy Native agent")

        activity.findViewById<TextView>(R.id.text)?.text = "Created"
        agentLog?.info("Legacy agent was created")

    }

    fun onStart() {
        legacyAgent?.start(activity?.applicationContext)
        activity.findViewById<TextView>(R.id.text)?.text = "Started"
        agentLog?.info("Legacy agent has started")
    }

    fun onStop() {
        activity.findViewById<TextView>(R.id.text)?.text = "Stopped"
        agentLog?.info("Legacy agent has stopped")
    }

    fun onDestroy() {
        onStop();
        activity.findViewById<TextView>(R.id.text)?.text = "Destroyed"
        agentLog?.info("Legacy agent destrpyed")
    }

}