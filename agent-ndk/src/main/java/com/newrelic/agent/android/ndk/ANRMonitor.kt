/**
 * Copyright 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.app.ActivityManager
import android.app.ActivityManager.ProcessErrorStateInfo
import android.content.Context
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.os.Process
import com.newrelic.agent.android.agentdata.AgentDataController
import com.newrelic.agent.android.stats.StatsEngine
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/**
 * An secondary approach to detecting main UI thread ANR conditions during runtime
 * from the main looper. The assumption here is a runnable task should pass
 * through the main looper (UI handler) within a reasonable ANR latency period.
 *
 * As of Android TIRAMISU, for regular apps this method will only return
 * ProcessErrorStateInfo records for the processes running as the caller's uid,
 * unless the caller has the permission Manifest.permission.DUMP.
 */
open class ANRMonitor {

    var future: Future<*>? = null
    val handler = Handler(Looper.getMainLooper())
    val monitorThread = HandlerThread("NR-ANR-Monitor")
    val executor: ExecutorService = Executors.newSingleThreadExecutor()
    var anrSampleCnt: AtomicInteger = AtomicInteger(ANR_SAMPLE_CNT)
    val anrMonitorRunner = Runnable {
        monitorThread.start()

        val runner = WaitableRunner()

        while (!Thread.interrupted()) {

            synchronized(runner) {
                try {
                    if (!handler.post(runner)) {
                        // post() returns false on failure, usually because the
                        // looper processing the message queue is exiting
                        AgentNDK.log.debug("Could not post the waitable runner to the main UI handler")
                        return@Runnable
                    }

                    val tStart = System.currentTimeMillis()
                    runner.wait(ANR_TIMEOUT)

                    if (runner.inANRState()) {
                        if (0 == anrSampleCnt.decrementAndGet()) {
                            AgentNDK.log.debug("ANR monitor is blocked, ANR detected")
                            createANRReport()
                            anrSampleCnt.set(ANR_SAMPLE_CNT)
                        }
                    } else {
                        // park the monitor for the remaining period
                        val tRemaining = ANR_TIMEOUT - (System.currentTimeMillis() - tStart)
                        Thread.sleep(Math.max(0, tRemaining))
                    }

                } catch (e: InterruptedException) {
                    AgentNDK.log.error("ANR monitor caught $e")

                } finally {
                    runner.reset()
                }
            }
        }

        monitorThread.quitSafely()
    }

    fun createANRReport(anrAsJson: String? = null) {
        StatsEngine.get().inc(AgentNDK.Companion.MetricNames.SUPPORTABILITY_ANR_DETECTED)

        // save the UI thread, aka the thread that is blocking
        val exception = NativeException(anrAsJson)

        // and weave in the JVM stacktrace
        exception.stackTrace = Looper.getMainLooper().thread.stackTrace

        // Poll the activity manager for details, then send the ANR as a HEx event
        reportWithRetry(exception)
    }

    private fun hasNativeStackFrames(stackTrace: Array<StackTraceElement>?): Boolean {
        stackTrace?.apply {
            return find { it.isNativeMethod } != null
        }
        return false
    }

    internal fun reportWithRetry(exception: NativeException) {
        val handler = Handler(monitorThread.looper)
        val retries = AtomicInteger(10)
        val attributes: HashMap<String?, Any?> = HashMap()

        attributes[AgentNDK.Companion.AnalyticsAttribute.APPLICATION_PLATFORM_ATTRIBUTE] = "native"
        attributes[AgentNDK.Companion.AnalyticsAttribute.APPLICATION_NOT_RESPONDING_ATTRIBUTE] = "true"

        exception.cause?.apply {
            attributes["cause"] = this.message
        }

        exception.nativeStackTrace?.apply {
            crashedThread?.apply {
                attributes["crashingThreadId"] = threadId
            }
            threads.apply {
                attributes["nativeThreads"] = this
            }
            exceptionMessage?.apply {
                attributes["exceptionMessage"] = this
            }
        }

        handler.post(
                object : Runnable {
                    override fun run() {
                        getProcessErrorStateInfoOrNull()?.apply {
                            attributes["pid"] = pid
                            attributes["processName"] = processName
                            attributes["shortMsg"] = shortMsg
                            attributes["longMsg"] = longMsg
                            attributes["stackTrace"] = stackTrace
                            attributes["tag"] = tag
                            when (condition) {
                                ProcessErrorStateInfo.NOT_RESPONDING -> {
                                    attributes["reason"] = 6
                                    /** ApplicationExitInfo.REASON_ANR */
                                }

                                ProcessErrorStateInfo.CRASHED -> {
                                    attributes["reason"] = 4
                                    /** ApplicationExitInfo.REASON_CRASH */
                                }
                            }
                            retries.set(0)
                        }

                        when (retries.getAndDecrement()) {
                            0 -> {
                                AgentNDK.log.debug(exception.toString())
                                AgentNDK.log.debug("ANR monitor notified. Posting ANR report as handled exception[${exception.javaClass.simpleName}]")
                                if (!AgentDataController.sendAgentData(exception, attributes)) {
                                    exception.printStackTrace()
                                }
                                AgentNDK.log.debug("ANR report created")
                            }

                            else -> {
                                handler.postDelayed(this, 500)
                            }
                        }
                    }
                })
    }

    fun startMonitor() {
        if (isRunning()) {
            stopMonitor()
        }
        future = executor.submit(anrMonitorRunner)
        AgentNDK.log.debug("ANR monitor started with [$ANR_TIMEOUT] ms delay")
    }

    fun stopMonitor() {
        future?.cancel(true)
        when (future?.isDone) {
            false -> future?.get()
            else -> {}
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
        // The default Android ANR timeout is about ~5 seconds, but use a generous value
        val ANR_TIMEOUT = TimeUnit.MILLISECONDS.convert(6, TimeUnit.SECONDS)

        // wait 2 consecutive ANRs before recording
        const val ANR_SAMPLE_CNT = 2

        // Running in a debugger?
        // val inDebugger = (Debug.isDebuggerConnected() || Debug.waitingForDebugger())

        @Volatile
        var anrMonitor: ANRMonitor? = null

        @JvmStatic
        fun getInstance() = anrMonitor ?: synchronized(this) {
            anrMonitor ?: ANRMonitor().also { anrMonitor = it }
        }

        @JvmStatic
        fun getProcessErrorStateInfoOrNull(): ActivityManager.ProcessErrorStateInfo? {
            val am = runCatching {
                AgentNDK.getInstance().managedContext?.context!!.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
            }.getOrNull()

            try {
                am?.processesInErrorState?.apply {
                    return firstOrNull { it.pid == Process.myPid() }
                }
            } catch (e: Exception) {
                AgentNDK.log.error(e.toString())
            }

            return null
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
                notify()
            }

            @Synchronized
            fun blocked(): Boolean {
                return !signaled
            }

            @Synchronized
            fun reset() {
                signaled = false
                notifyAll()
            }

            fun inANRState(): Boolean {
                if (blocked()) {
                    getProcessErrorStateInfoOrNull()?.apply {
                        return condition == ProcessErrorStateInfo.NOT_RESPONDING
                    }
                }

                return false
            }

        }
    }
}