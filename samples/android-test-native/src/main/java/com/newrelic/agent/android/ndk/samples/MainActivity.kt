/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples

import android.content.ClipData
import android.content.ClipboardManager
import android.os.Bundle
import android.view.*
import android.widget.Spinner
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.newrelic.agent.android.ndk.AgentNDK
import com.newrelic.agent.android.ndk.AgentNDKListener
import com.newrelic.agent.android.ndk.NativeCrash
import com.newrelic.agent.android.ndk.NativeException
import java.util.*
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit

class MainActivity : AppCompatActivity(), AgentNDKListener {
    private var threadPool: ScheduledExecutorService = Executors.newScheduledThreadPool(1)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        threadPool.schedule(Runnable() {
            // boomSegVInBackground();
        }, 10000, TimeUnit.MILLISECONDS)

        findViewById<TextView>(R.id.text).text = installNative()

        // Change these to suite your testing
        val buildId = UUID.randomUUID().toString()
        val sessionId = UUID.randomUUID().toString()
        val reportTTL = TimeUnit.SECONDS.convert(3, TimeUnit.DAYS)

        AgentNDK.Builder(this)
            .withReportListener(this)
            .withBuildId(buildId)
            .withSessionId(sessionId)
            .withExpiration(reportTTL)
            .build()

        AgentNDK.getInstance().start()

        val btnSignal = findViewById<TextView>(R.id.btnSignal)
        val btnThrow = findViewById<TextView>(R.id.btnThrow)
        val btnThreadedCrash = findViewById<TextView>(R.id.btnThreadedCrash)
        val btnANR = findViewById<TextView>(R.id.btnANR)

        val signalSpinner: Spinner = findViewById(R.id.signalSpinner)
        val exceptionSpinner: Spinner = findViewById(R.id.exceptionSpinner)

        btnSignal.setOnClickListener() {
            val item = signalSpinner.selectedItem as String
            val (name, signo) = "^(SIG[A-Z]+) \\((\\d.*)\\)".toRegex().find(item)!!.destructured
            Toast.makeText(this, "Raising $name", Toast.LENGTH_SHORT).show()
            raiseSignal(signo.toInt())
        }

        btnThrow.setOnClickListener() {
            val item = exceptionSpinner.selectedItem as String
            Toast.makeText(this, "Throwing $item", Toast.LENGTH_SHORT).show()
            raiseException(exceptionSpinner.selectedItemPosition)
        }

        btnThreadedCrash.setOnClickListener() {
            Toast.makeText(this, "Starting detached crashing thread(s)", Toast.LENGTH_LONG).show()
            backgroundCrash()
        }

        btnANR.setOnClickListener() {
            // waste enough time on the main thread to trigger ANR
            Toast.makeText(this, "Blocking the main tread for 10 secs", Toast.LENGTH_LONG).show()
            runOnUiThread() {
                Thread.sleep(10000)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        AgentNDK.getInstance().stop()
    }

    override fun onCreateContextMenu(
        menu: ContextMenu, v: View,
        menuInfo: ContextMenu.ContextMenuInfo
    ) {
        super.onCreateContextMenu(menu, v, menuInfo)
        val inflater: MenuInflater = menuInflater
        inflater.inflate(R.menu.settings, menu)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        val inflater: MenuInflater = menuInflater
        inflater.inflate(R.menu.settings, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.dump_stack -> {
                val backtrace = AgentNDK.getInstance().dumpStack()
                findViewById<TextView>(R.id.text).text = backtrace
                // and put it on the clipboard for debugging
                postToClipboard(backtrace)
                true
            }
            R.id.crash_me -> {
                AgentNDK.getInstance().crashNow()
                true
            }
            R.id.flush_reports -> {
                AgentNDK.getInstance().flushPendingReports()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private external fun installNative(): String
    private external fun raiseSignal(signo: Int): Void
    private external fun raiseException(exceptionNameIndex: Int): Void
    private external fun backgroundCrash(
        signo: Int = 4,
        sleepSec: Int = 5,
        threadCnt: Int = 4,
        detached: Boolean = true
    ): Void

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    override fun onNativeCrash(crashAsString: String?): Boolean {
        val exception = NativeException(crashAsString)
        val crash = NativeCrash(exception)
        val crashStr = crash.toJsonString()
        findViewById<TextView>(R.id.text).text = crashStr
        postToClipboard(crashStr)

        return true
    }

    override fun onNativeException(exceptionAsString: String?): Boolean {
        val exception = NativeException(exceptionAsString!!)
        val exStr = exception.stackTraceToString()
        findViewById<TextView>(R.id.text).text = exStr
        postToClipboard(exStr)
        return true
    }

    override fun onApplicationNotResponding(anrAsString: String?): Boolean {
        val exception = NativeException(anrAsString!!)
        val exStr = exception.stackTraceToString()
        findViewById<TextView>(R.id.text).text = exStr
        postToClipboard(exStr)
        return true
    }

    private fun postToClipboard(str: String) {
        val clipboard: ClipboardManager =
            getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText("backtrace", str)
        clipboard.setPrimaryClip(clip)
    }


}