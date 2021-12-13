/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.os.Debug
import android.os.Handler
import android.os.Looper
import com.newrelic.agent.android.agentdata.AgentDataController
import com.newrelic.agent.android.util.NamedThreadFactory
import java.util.*
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.TimeUnit

/**
 * An secondary approach to detecting ANR conditions during runtime
 * from the JVM. The assumption here is a runnable task should pass
 * through the main looper (UI handler) within the default ANR latency period.
 */
open class ANRMonitor {

    var future: Future<*>? = null
    val handler: Handler = Handler(Looper.getMainLooper())
    var disableWhileDebugging = false
    val executor = Executors.newSingleThreadExecutor(
        NamedThreadFactory("NR-ANRMonitor")
    )

    private val anrMonitorRunner = Runnable {
        while (!Thread.interrupted()) {
            try {
                val runner = WaitableRunner()

                synchronized(runner) {
                    if (!handler.post(runner)) {
                        // post returns false on failure, usually because the
                        // looper processing the message queue is exiting
                        return@Runnable
                    }

                    runner.wait(DEFAULT_ANR_TIMEOUT)

                    if (!runner.signaled) {
                        if (disableWhileDebugging &&
                            (Debug.isDebuggerConnected() || Debug.waitingForDebugger())
                        ) {
                            return@synchronized
                        }

                        val attributes: HashMap<String?, Any?> = object : HashMap<String?, Any?>() {
                            init {
                                put(AgentNDK.Companion.AnalyticsAttribute.APPLICATION_PLATFORM_ATTRIBUTE, "native")
                                put("ANR", "true")
                            }
                        }

                        val exceptionToHandle: Exception = NativeException("Application not responding")
                        if (!AgentDataController.sendAgentData(exceptionToHandle, attributes)) {
                            AgentNDK.log.error("AgentDataController not initialized")
                            stopMonitor()
                        }

                        runner.wait()
                    }

                    Thread.yield()
                }
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }
        }
    }

    /**
     * Ignore detected ANRs when debugging
     */
    fun disableWhileDebugging(): ANRMonitor {
        disableWhileDebugging = true
        return this
    }

    fun startMonitor() {
        stopMonitor()
        future = executor.submit(anrMonitorRunner)
        AgentNDK.log.debug("ANR monitor started with [$DEFAULT_ANR_TIMEOUT]ms delay")
    }

    fun stopMonitor() {
        future?.cancel(true)
        when (future?.isDone) {
            false -> future?.get()
        }
        future?.apply { future = null }
        AgentNDK.log.debug("ANR monitor stopped")
    }

    fun isRunning(): Boolean {
        future?.apply {
            return !(this.isCancelled || this.isDone)
        }
        return false
    }

    companion object {
        val DEFAULT_ANR_TIMEOUT = TimeUnit.MILLISECONDS.convert(5, TimeUnit.SECONDS)

        @Volatile
        var anrMonitor: ANRMonitor? = null

        @JvmStatic
        fun getInstance() = anrMonitor ?: synchronized(this) {
            anrMonitor ?: ANRMonitor().also { anrMonitor = it }
        }

        /**
         * WaitableRunner provides the means of detecting if a task
         * has travelled through the looper, or has been blocked
         */
        @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN")
        internal class WaitableRunner : Object(), Runnable {
            var signaled: Boolean = false

            @Synchronized
            override fun run() {
                signaled = true
                notifyAll()
            }
        }
    }
}