/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import com.newrelic.agent.android.logging.AgentLog
import com.newrelic.agent.android.metric.MetricNames
import com.newrelic.agent.android.stats.StatsEngine
import java.io.File
import java.util.*
import java.util.concurrent.locks.ReentrantLock

open class AgentNDK(managedContext: ManagedContext? = ManagedContext()) {
    val lock = ReentrantLock()
    var managedContext: ManagedContext? = managedContext

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
        var log: AgentLog = NativeLogger()

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

        // flushPendingReports()

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
            managedContext?.reportsDir?.run {
                log.info("Flushing native reports from [${absolutePath}]")
                if (exists() && canRead()) {
                    listFiles()?.let {
                        for (report in it) {
                            if (report.lastModified() < (System.currentTimeMillis() - managedContext?.reportTTL!!)) {
                                log.error("Report [${report.name}] is too old, deleting...")
                                report.deleteOnExit()
                                continue;
                            }
                            try {
                                if (postReport(report)) {
                                    log.debug("Native report [${report.name}] submitted to New Relic")
                                }
                            } catch (e: Exception) {
                                log.warning("Failed to parse/write native report [${report.name}: $e")
                            }
                        }
                    }
                } else {
                    log.warning("Native report directory [${absolutePath}] does not exist or not readable")
                }
            } ?: run {
                log.warning("Report directory has not been provided")
            }
        } finally {
            lock.unlock()
        }
    }

    private fun postReport(report: File): Boolean {
        if (report.exists()) {
            log.info("Posting native report data from [${report.absolutePath}]")
            managedContext?.nativeReportListener?.apply {
                var consumed = false

                when {
                    report.name.startsWith("crash-", true) -> {
                        consumed = onNativeCrash(report.readText(Charsets.UTF_8))
                    }
                    report.name.startsWith("ex-", true) -> {
                        consumed = onNativeException(report.readText(Charsets.UTF_8))
                    }
                    report.name.startsWith("anr-", true) -> {
                        consumed = onApplicationNotResponding(report.readText(Charsets.UTF_8))
                    }
                }

                if (consumed) {
                    if (report.delete()) {
                        log.info("Deleted native report data [${report.absolutePath}")
                    } else {
                        log.error("Failed to delete native report [${report.absolutePath}]")
                    }
                }
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

        init {
            loadAgent()
        }

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

        fun withLogger(agentLog: AgentLog): Builder {
            AgentNDK.log = agentLog
            return this
        }

        fun build(): AgentNDK {
            managedContext.reportsDir?.mkdirs()
            agentNdk = AgentNDK(managedContext)
            return getInstance()
        }
    }

}