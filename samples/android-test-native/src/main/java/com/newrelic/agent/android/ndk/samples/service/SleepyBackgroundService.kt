/*
 * Copyright (c) 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples.service

import android.app.Service
import android.content.Intent
import android.os.*
import android.widget.Toast
import com.newrelic.agent.android.NewRelic
import java.util.concurrent.atomic.AtomicLong

class SleepyBackgroundService : Service() {
    companion object {
        val TAG = "SleepyBackgroundService"
        val seqNo: AtomicLong = AtomicLong(0)
    }

    private var serviceLooper: Looper? = null
    private var serviceHandler: ServiceHandler? = null
    private var startMode: Int = START_REDELIVER_INTENT     // indicates how to behave if the service is killed
    private var binder: IBinder? = SleepyBinder()                 // interface for clients that bind
    private var allowRebind: Boolean = true                 // indicates whether onRebind should be used

    // Handler that receives messages from the thread
    private inner class ServiceHandler(looper: Looper) : Handler(looper) {

        override fun handleMessage(msg: Message) {
            try {
                // do some work:
                Thread.sleep(5000 + ((Math.random() * 8000).toLong()))
                NewRelic.recordBreadcrumb("ServiceBreadcrumb")

            } catch (e: Exception) {
                // Restore interrupt status.
                Thread.currentThread().interrupt()
                e.printStackTrace()
            }

            // Stop the service using the startId, so that we don't stop
            // the service in the middle of handling another job
            stopSelf(msg.arg1)
        }
    }

    override fun onCreate() {
        // Start up the thread running the service.  Note that we create a
        // separate thread because the service normally runs in the process's
        // main thread, which we don't want to block.  We also make it
        // background priority so CPU-intensive work will not disrupt our UI.
        HandlerThread("ServiceStartArguments", Process.THREAD_PRIORITY_BACKGROUND).apply {
            start()
            // Get the HandlerThread's Looper and use it for our Handler
            serviceLooper = looper
            serviceHandler = ServiceHandler(looper)
            Thread.sleep(12 * 1000)
        }
        NewRelic.setAttribute("serviceState", "created")
    }

    override fun onStartCommand(intent: Intent, flags: Int, startId: Int): Int {
        Toast.makeText(this, "BackgroundMonitoringService starting", Toast.LENGTH_LONG).show()
        Thread.sleep(30 * 1000)

        // For each start request, send a message to start a job and deliver the
        // start ID so we know which request we're stopping when we finish the job
        serviceHandler?.obtainMessage()?.also { msg ->
            msg.arg1 = startId
            serviceHandler?.sendMessage(msg)
        }

        val attrs = HashMap<String, Any>().apply {
            intent.apply {
                action?.let { it -> put("action", it) }
                put("flags", flags)
                categories?.joinToString()?.let { it1 -> put("category", it1) }
            }
            put("startId", startId)
            put("message", "onStartCommand")
        }
        NewRelic.recordCustomEvent(TAG, attrs)
        NewRelic.setAttribute("serviceState", "started")

        /**
         * START_STICKY_COMPATIBILITY: Does not guarantee that {@link #onStartCommand} will
         * be called again after being killed.
         *
         * START_STICKY: If the system kills the service after onStartCommand() returns,
         * recreate the service and call onStartCommand(), but do not redeliver the last intent.
         * Instead, the system calls onStartCommand() with a null intent unless there are pending
         * intents to start the service. In that case, those intents are delivered. This is
         * suitable for media players (or similar services) that are not executing commands
         * but are running indefinitely and waiting for a job.
         *
         * START_NOT_STICKY: If the system kills the service after onStartCommand() returns,
         * do not recreate the service unless there are pending intents to deliver.
         * This is the safest option to avoid running your service when not necessary and when
         * your application can simply restart any unfinished jobs.
         *
         * START_REDELIVER_INTENT: If the system kills the service after onStartCommand() returns,
         * recreate the service and call onStartCommand() with the last intent that was delivered
         * to the service. Any pending intents are delivered in turn. This is suitable for services
         * that are actively performing a job that should be immediately resumed, such as
         * downloading a file
         */
        // If we get killed, after returning from here, restart
        return startMode
    }

    override fun onDestroy() {
        Toast.makeText(this, "BackgroundMonitoringService done", Toast.LENGTH_LONG).show()
        Thread.sleep(8000)
        val attrs = HashMap<String, Any>().apply {
            put("serviceState", "destroyed")
        }
        NewRelic.recordCustomEvent(TAG, attrs)
        NewRelic.setAttribute("serviceState", "destroyed")
    }

    override fun onBind(intent: Intent): IBinder? {
        // A client is binding to the service with bindService()
        val attrs = HashMap<String, Any>().also {
            it.put("categories", intent.categories)
            it.put("flags", intent.flags)
            it.put("message", "onBind")
        }
        NewRelic.recordCustomEvent(TAG, attrs)
        NewRelic.setAttribute("serviceState", "bound")

        Thread.sleep(8 * 1000)
        return binder
    }

    override fun onUnbind(intent: Intent): Boolean {
        Thread.sleep(8000)
        // All clients have unbound with unbindService()
        NewRelic.setAttribute("serviceState", "unbound")
        return allowRebind
    }

    override fun onRebind(intent: Intent) {
        // A client is binding to the service with bindService(),
        // after onUnbind() has already been called
        NewRelic.setAttribute("serviceState", "rebound")
    }

}