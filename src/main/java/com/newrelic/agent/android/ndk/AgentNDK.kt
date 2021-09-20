/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import com.newrelic.agent.android.logging.AgentLog
import com.newrelic.agent.android.logging.AgentLogManager
import com.newrelic.agent.android.logging.ConsoleAgentLog
import com.newrelic.agent.android.metric.MetricNames
import com.newrelic.agent.android.stats.StatsEngine
import java.io.File
import java.util.*
import java.util.concurrent.locks.ReentrantLock

open class AgentNDK(managedContext: ManagedContext? = ManagedContext()) {
    var log = AgentLogManager.getAgentLog()
    val lock = ReentrantLock()
    var managedContext = managedContext

    /**
     * API methods callable from agent
     **/
    external fun nativeStart(context: ManagedContext? = null): Boolean
    external fun nativeStop()
    external fun setNativeContext(context: ManagedContext)
    external fun crashNow(cause: String? = "This is a demonstration native crash courtesy of New Relic")
    external fun dumpStack(): String
    external fun isRootedDevice(): Boolean

    companion object {
        val log: AgentLog = ConsoleAgentLog()

        @Volatile
        private var agentNdk: AgentNDK? = null

        @JvmStatic
        fun loadAgent(): Boolean {
            try {
                System.loadLibrary("agent-ndk")
                log.info("Agent NDK loaded")
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_CRASH)

            } catch (e: UnsatisfiedLinkError) {
                if ("Dalvik".equals(System.getProperty("java.vm.name"))) {
                    StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_LOAD_ERR)
                    throw e
                }
            } catch (e: Exception) {
                log.info("Agent NDK load failed: " + e.localizedMessage)
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_LOAD_ERR)
                return false
            }

            return true
        }

        @JvmStatic
        fun getInstance() = agentNdk ?: synchronized(this) {
            agentNdk ?: AgentNDK().also { agentNdk = it }
        }

        @JvmStatic
        fun shutdown(hardKill: Boolean = false) {
            if (hardKill) {
                TODO()
            }
            getInstance().stop()
        }
    }

    fun start(): Boolean {
        if (managedContext?.anrMonitor == true) {
            ANRMonitor.getInstance().startMonitor()
        }

        try {
            lock.lock()
            flushPendingReports()
        } finally {
            lock.unlock()
        }

        return nativeStart(managedContext!!)
    }

    fun stop() {
        if (managedContext?.anrMonitor == true) {
            ANRMonitor.getInstance().stopMonitor()
        }
        nativeStop()
    }

    fun flushPendingReports() {
        lock.lock()
        try {
            val reportsDir = managedContext?.reportsDir!!
            if (reportsDir.exists() && reportsDir.canRead()) {
                val reportslist = reportsDir.listFiles()
                if (reportslist != null) {
                    for (report in reportslist) {
                        if (postReport(report)) {
                            // LOG it
                        }
                    }
                }
            } else {
                log.warning("Report directory [${reportsDir}] does not exist or not readable")
            }
        } catch (ex: Exception) {
            log.warning("Failed to parse/write pending reports: $ex")
        } finally {
            lock.unlock()
        }
    }

    private fun postReport(report: File): Boolean {
        if (report.exists()) {
            log.info("Posting native report data: " + report.absolutePath)
            managedContext?.nativeReportListener?.apply {
                if (report.name.startsWith("crash-", true)) {
                    onNativeCrash(report.readText(Charsets.UTF_8))
                } else if (report.name.startsWith("ex-", true)) {
                    onNativeException(report.readText(Charsets.UTF_8))
                } else if (report.name.startsWith("anr-", true)) {
                    onApplicationNotResponding(report.readText(Charsets.UTF_8))
                }
                log.info("Deleting native report data: " + report.absolutePath)
                report.delete()
            }
            return !report.exists()
        }
        return false
    }

    /**
     * Builder to install or replace the agent instance
     */
    class Builder(val context: Context) {
        var managedContext = ManagedContext(context)

        fun withBuildId(buildId: UUID): Builder {
            managedContext.buildId = buildId.toString()
            return this
        }

        fun withSessionId(sessionId: UUID): Builder {
            managedContext.sessionId = sessionId.toString()
            return this
        }

        fun withStorageDir(storageRootDir: File): Builder {
            managedContext.reportsDir = managedContext.getNativeReportsDir(storageRootDir);
            managedContext.reportsDir?.mkdirs()
            return this
        }

        fun withReportListener(ndkListener: AgentNDKListener): Builder {
            managedContext.nativeReportListener = ndkListener
            return this
        }

        fun withManagedContext(managedContext: ManagedContext): Builder {
            this.managedContext = managedContext
            return this
        }

        fun build(): AgentNDK {
            managedContext.reportsDir?.mkdirs()
            agentNdk = AgentNDK(managedContext)
            return getInstance()
        }
    }

}