/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples

import android.annotation.SuppressLint
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.os.Bundle
import android.view.*
import android.widget.Spinner
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.floatingactionbutton.FloatingActionButton
import com.newrelic.agent.android.ndk.AgentNDK
import com.newrelic.agent.android.ndk.AgentNDKListener
import com.newrelic.agent.android.ndk.NativeCrash
import com.newrelic.agent.android.ndk.NativeException
import com.newrelic.agent.android.ndk.samples.service.SleepyBackgroundService
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit

class MainActivity : AppCompatActivity(), AgentNDKListener {
    private var threadPool: ScheduledExecutorService = Executors.newScheduledThreadPool(1)
    private var newRelicAgent: NewRelicAgent? = null

    @SuppressLint("SetTextI18n")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        threadPool.schedule(Runnable() {
            // boomSegVInBackground();
        }, 10000, TimeUnit.MILLISECONDS)

        findViewById<TextView>(R.id.text).text = installNative()

        newRelicAgent = NewRelicAgent(this)
        newRelicAgent?.onCreate()

        val btnSignal = findViewById<TextView>(R.id.btnSignal)
        val btnThrow = findViewById<TextView>(R.id.btnThrow)
        val btnThreadedCrash = findViewById<TextView>(R.id.btnThreadedCrash)
        val btnANR = findViewById<TextView>(R.id.btnANR)

        val signalSpinner: Spinner = findViewById(R.id.signalSpinner)
        val exceptionSpinner: Spinner = findViewById(R.id.exceptionSpinner)
        val anrSpinner: Spinner = findViewById(R.id.anrSpinner)

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

        btnANR.setOnClickListener() {
            val item = anrSpinner.selectedItem as String
            Toast.makeText(this, "Forcing ANR with $item", Toast.LENGTH_SHORT).show()
            when (anrSpinner.selectedItemPosition) {
                0 -> anrUIThread()
                1 -> anrContentProvider()
                2 -> anrService()
            }
        }

        btnThreadedCrash.setOnClickListener() {
            Toast.makeText(this, "Starting detached crashing thread(s)", Toast.LENGTH_LONG).show()
            backgroundCrash()
        }
    }

    // Activity blocking Main UI thread ANR
    fun anrUIThread() {
        runOnUiThread() {
            findViewById<TextView>(R.id.text).text = "Blocking the UI thread"
            val btnANR = findViewById<TextView>(R.id.btnANR)
            Toast.makeText(this, "Blocking the main Activity thread", Toast.LENGTH_LONG).show()
            btnANR.text = "Sleeping..."    // needs some user input
            btnANR.setPressed(true);
            btnANR.invalidate();
            Thread.sleep(8000)
            btnANR.text = getString(R.string.anr)
        }
    }

    // Slow content provider ANR
    fun anrContentProvider() {
        findViewById<TextView>(R.id.text).text = "Blocking the main tread through Context Provider"
        Toast.makeText(this, "Blocking the main tread through Context Provider", Toast.LENGTH_LONG).show()
        var contentProvider = getContentResolver()
        val contentType = contentProvider?.getType(CheeseyContentProvider.CONTENT_URI)

        contentProvider.query(
            CheeseyContentProvider.CONTENT_URI,
            null, null, null, null
        ).apply {
            val stock = StringBuilder("CONTENT_URI: " + contentType + "\n");
            if (moveToFirst()) {
                while (!isAfterLast()) {
                    stock.append("\n["
                                + getString(getColumnIndex(CheeseyContentProvider.country))
                                + "] "
                                + getString(getColumnIndex(CheeseyContentProvider.name))
                    )
                    moveToNext();
                }
            } else {
                stock.append("No cheese for you")
            }
            runOnUiThread() {
                findViewById<TextView>(R.id.text).text = stock
            }
        }
    }

    // Slow service ANR
    fun anrService() {
        findViewById<TextView>(R.id.text).text = "Blocking through slow service"

        threadPool.execute(Runnable {
            Thread.sleep((Math.random() * 1234).toLong())
            AndroidTestNative.alarmProvider.setWakeyWakey()
        })

        threadPool.scheduleWithFixedDelay(Runnable {
            val intent = Intent(this, SleepyBackgroundService::class.java)
            intent.putExtra("seqNo", SleepyBackgroundService.seqNo.getAndIncrement())
            startService(intent)
        }, 0, (Math.random() * 1234).toLong(), TimeUnit.MILLISECONDS)
    }

    override fun onDestroy() {
        super.onDestroy()
        newRelicAgent?.onDestroy()
    }

    override fun onStart() {
        super.onStart()
        newRelicAgent?.onStart()

        // set an alarm for every 3 min that starts in 30 seconds
        AndroidTestNative.alarmProvider.setInexact3MinRepeatingAlarm(30000)
    }

    override fun onStop() {
        super.onStop()
        newRelicAgent?.onStop()
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
            R.id.cpu_sample -> {
                val sample = AgentNDK.getInstance().getProcessStat()
                findViewById<TextView>(R.id.text).text = sample
                postToClipboard(sample)
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

    private external fun triggerNativeANR(): Void

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