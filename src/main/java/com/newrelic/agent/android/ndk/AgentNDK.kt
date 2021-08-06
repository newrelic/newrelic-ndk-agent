/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import com.newrelic.agent.android.logging.AgentLog
import com.newrelic.agent.android.logging.AgentLogManager
import com.newrelic.agent.android.metric.MetricNames
import com.newrelic.agent.android.stats.StatsEngine


open class AgentNDK {

    /**
     * API methods callable from agent
     **/
    external fun initialize(): Boolean
    external fun shutdown(hardKill: Boolean = false): Void
    external fun crashNow(cause: String? = "This is a demonstration native crash courtesy of New Relic"): Void
    external fun dumpstack(): String

    companion object {

        class Builder {

        }

        private val log: AgentLog = AgentLogManager.getAgentLog()

        init {
            // TODO: loadAgent()
        }

        @JvmStatic
        fun loadAgent(): Boolean {
            try {
                System.loadLibrary("agent-ndk")
                log.info("Agent NDK loaded")
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_CRASH)

            } catch (e: UnsatisfiedLinkError) {
                // only ignore exception in non-android env
                if ("Dalvik".equals(System.getProperty("java.vm.name"))) {
                    throw e
                }
            } catch (e: Exception) {
                log.info("Agent NDK load failed: " + e.localizedMessage)
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_CRASH_ERR)
                return false
            }

            return true
        }

        @Volatile
        private var agentNdk: AgentNDK? = null

        @JvmStatic
        fun getInstance() = agentNdk ?: synchronized(this) {
            agentNdk ?: AgentNDK().also { agentNdk = it }
        }
    }

    fun getNativeLibraryDir(context: Context) {
        val packageName = context.packageName
        val packageManager = context.packageManager

        val ainfo: ApplicationInfo =
            packageManager.getApplicationInfo(packageName, PackageManager.GET_SHARED_LIBRARY_FILES)
        log.debug("native library dir " + ainfo.nativeLibraryDir)
    }

}