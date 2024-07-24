/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import android.util.Log
import com.newrelic.agent.android.logging.AgentLog
import com.newrelic.agent.android.logging.ConsoleAgentLog
import com.newrelic.agent.android.stats.StatsEngine
import com.scottyab.rootbeer.RootBeer
import java.io.File
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.ReentrantLock


open class AgentNDK(val managedContext: ManagedContext? = ManagedContext()) {
    /**
     * API methods
     **/
    external fun nativeStart(context: ManagedContext? = null): Boolean
    external fun nativeStop(): Boolean
    external fun nativeSetContext(context: ManagedContext)

    external fun crashNow(cause: String? = "This is a demonstration native crash courtesy of New Relic")
    external fun dumpStack(): String
    external fun getProcessStat(): String

    companion object {
        internal interface AnalyticsAttribute {
            companion object {
                const val APPLICATION_PLATFORM_ATTRIBUTE = "platform"
                const val APPLICATION_NOT_RESPONDING_ATTRIBUTE = "ANR"
            }
        }

        internal interface MetricNames {
            companion object {
                const val SUPPORTABILITY_NATIVE_ROOT = "Supportability/AgentHealth/NativeReporting"
                const val SUPPORTABILITY_NATIVE_CRASH = "$SUPPORTABILITY_NATIVE_ROOT/Crash"
                const val SUPPORTABILITY_NATIVE_LOAD_ERR = "$SUPPORTABILITY_NATIVE_ROOT/Error/LoadLibrary"
                const val SUPPORTABILITY_ANR_DETECTED = "$SUPPORTABILITY_NATIVE_ROOT/ANR/Detected"
            }
        }

        val lock = ReentrantLock()

        @Volatile
        var log: AgentLog = ConsoleAgentLog()

        @Volatile
        var agentNdk: AgentNDK? = null

        @JvmStatic
        fun loadAgent(): Boolean {
            try {
                System.loadLibrary("agent-ndk")
                log.info("Agent NDK loaded")
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_CRASH)

            } catch (e: Exception) {
                log.error("Agent NDK load failed: " + e.localizedMessage)
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_LOAD_ERR)
                return false

            } catch (e: UnsatisfiedLinkError) {
                log.error("Agent NDK load failed: " + e.localizedMessage)
                StatsEngine.get().inc(MetricNames.SUPPORTABILITY_NATIVE_LOAD_ERR)
                return false
            }

            return true
        }

        @JvmStatic
        fun getInstance() = agentNdk ?: synchronized(this) {
            agentNdk ?: AgentNDK().also { agentNdk = it }
        }
    }

    fun start(): Boolean {
        if (managedContext?.anrMonitor == true) {
            ANRMonitor.getInstance().startMonitor()
            Log.d("AgentNDK", "Main thread ANR monitor started")
        }

        return nativeStart(managedContext!!)
    }

    fun stop(): Boolean {
        if (managedContext?.anrMonitor == true) {
            ANRMonitor.getInstance().stopMonitor()
        }

        return nativeStop()
    }

    fun flushPendingReports() {
        lock.lock()
        try {
            managedContext?.reportsDir?.run {
                log.info("Flushing native reports from [${absolutePath}]")
                if (exists() && canRead()) {
                    listFiles()?.let {
                        for (report in it) {
                            try {
                                if (postReport(report)) {
                                    log.info("Native report [${report.name}] submitted to New Relic")
                                    continue
                                }
                            } catch (e: Exception) {
                                log.warn("Failed to parse/write native report [${report.name}: $e")
                            }

                            val expirationTimeMs: Long = (System.currentTimeMillis() -
                                    TimeUnit.MILLISECONDS.convert(
                                        managedContext.expirationPeriod,
                                        TimeUnit.SECONDS
                                    ))

                            if (report.exists() && (report.lastModified() < expirationTimeMs)) {
                                log.info("Native report [${report.name}] has expired, deleting...")
                                report.deleteOnExit()
                            }
                        }
                    }
                } else {
                    log.warn("Native report directory [${absolutePath}] does not exist or not readable")
                }
            } ?: run {
                log.warn("Report directory has not been provided")
            }
        } finally {
            lock.unlock()
        }
    }

    protected fun postReport(report: File): Boolean {
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
                        log.debug("Deleted native report data [${report.absolutePath}]")
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
     * Methods to access native capabilities
     */

    fun isRooted(): Boolean {
        RootBeer(managedContext?.context).also {
            return it.isRooted
        }
    }


    /**
     * Builder to install or replace the agent instance
     */
    class Builder(val context: Context) {
        var managedContext = ManagedContext(context)

        init {
            loadAgent()
        }

        fun withBuildId(buildId: String): Builder {
            managedContext.buildId = buildId
            return this
        }

        fun withSessionId(sessionId: String): Builder {
            managedContext.sessionId = sessionId
            return this
        }

        fun withStorageDir(storageRootDir: File?): Builder {
            managedContext.reportsDir = managedContext.getNativeReportsDir(storageRootDir)
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

        /**
         * Sets the report expiration time, in seconds
         */
        fun withExpiration(expirationPeriod: Long): Builder {
            managedContext.expirationPeriod = expirationPeriod
            return this
        }

        fun withLogger(agentLog: AgentLog): Builder {
            log = agentLog
            return this
        }

        fun withANRMonitor(enabled: Boolean): Builder {
            managedContext.anrMonitor = enabled
            return this
        }

        fun build(): AgentNDK {
            managedContext.reportsDir?.mkdirs()
            agentNdk = AgentNDK(managedContext)
            return getInstance()
        }

    }

}
