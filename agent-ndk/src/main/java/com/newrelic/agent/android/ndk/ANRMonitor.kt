/**
 * Copyright 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.app.ActivityManager
import android.content.Context
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.os.Process
import com.newrelic.agent.android.agentdata.AgentDataController
import com.newrelic.agent.android.stats.StatsEngine
import java.util.*
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/**
 * An secondary approach to detecting ANR conditions during runtime
 * from the JVM. The assumption here is a runnable task should pass
 * through the main looper (UI handler) within the default ANR latency period.
 */
open class ANRMonitor {

    var future: Future<*>? = null
    val handler = Handler(Looper.getMainLooper())
    val monitorThread = HandlerThread("NR-ANRMonitor")
    val executor: ExecutorService = Executors.newSingleThreadExecutor()
    val anrMonitorRunner = Runnable {
        monitorThread.start()

        while (!Thread.interrupted()) {
            try {
                val runner = WaitableRunner()

                synchronized(runner) {
                    if (!handler.post(runner)) {
                        // post returns false on failure, usually because the
                        // looper processing the message queue is exiting
                        return@Runnable
                    }

                    runner.wait(ANR_TIMEOUT)

                    if (!runner.signaled) {
                        notify()
                        runner.wait()
                    }

                    Thread.yield()
                }
            } catch (e: InterruptedException) {
                // e.printStackTrace()
            }
        }
        monitorThread.quitSafely()
    }

    fun notify(anrAsJson: String? = null) {
        StatsEngine.get()
            .inc(AgentNDK.Companion.MetricNames.SUPPORTABILITY_ANR_DETECTED)

        // save the UI thread, aka the thread that is blocking
        val exception = NativeException(anrAsJson)

        // and weave in the JVM stacktrace
        exception.stackTrace = Looper.getMainLooper().thread.stackTrace

        // Poll the activity manager for details, then send the ANR as a HEx event
        reportWithRetry(exception)
    }

    internal fun isNativeTrace(stackTrace: Array<StackTraceElement>): Boolean {
        if (!stackTrace.isEmpty()) {
            return stackTrace[0].isNativeMethod
        }
        return true // false
    }

    internal fun getProcessErrorStateOrNull(): ActivityManager.ProcessErrorStateInfo? {
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

    internal fun reportWithRetry(exception: NativeException) {
        val handler = Handler(monitorThread.looper)
        val retries = AtomicInteger(250)
        val attributes: HashMap<String?, Any?> = HashMap<String?, Any?>()

        attributes.put(
            AgentNDK.Companion.AnalyticsAttribute.APPLICATION_PLATFORM_ATTRIBUTE,
            "native"
        )
        attributes.put(
            AgentNDK.Companion.AnalyticsAttribute.APPLICATION_NOT_RESPONDING_ATTRIBUTE,
            "true"
        )

        exception.cause?.apply {
            attributes.put("cause", this.message)
        }

        exception.nativeStackTrace?.apply {
            crashedThread?.apply {
                attributes.put("crashingThreadId", threadId)
            }
            threads.apply {
                attributes.put("threads", this)
            }
            exceptionMessage?.apply {
                attributes.put("exceptionMessage", this)
            }
        }

        handler.post(
            object : Runnable {
                override fun run() {
                    val anrDetails = getProcessErrorStateOrNull()

                    if (anrDetails != null) {
                        AgentNDK.log.debug("ANR monitor notified. Posting ANR report")

                        attributes.put("pid", anrDetails?.pid)
                        attributes.put("uid", anrDetails?.uid)
                        attributes.put("processName", anrDetails?.processName)
                        attributes.put("shortMsg", anrDetails?.shortMsg)
                        attributes.put("longMsg", anrDetails?.longMsg)
                        attributes.put("stackTrace", anrDetails?.stackTrace)
                        attributes.put("tag", anrDetails?.tag)
                        attributes.put("condition", anrDetails?.condition)

                        if (!AgentDataController.sendAgentData(exception, attributes)) {
                            exception.printStackTrace()
                        }

                    } else if (retries.getAndDecrement() > 0) {
                        handler.postDelayed(this, 50)

                    } else {
                        if (!AgentDataController.sendAgentData(exception, attributes)) {
                            exception.printStackTrace()
                        }
                    }
                }
            }
        )
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
        // The default Android ANR timeout is ~5 seconds.
        // Setting timeout to 1 second captures code execution leading
        // up to a reported ANR
        val ANR_TIMEOUT = TimeUnit.MILLISECONDS.convert(5, TimeUnit.SECONDS)

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