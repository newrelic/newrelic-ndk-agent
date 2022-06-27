package com.newrelic.agent.android.ndk.samples

import android.annotation.SuppressLint
import android.app.Application
import android.content.Intent
import com.newrelic.agent.android.ndk.samples.service.AlarmProvider
import com.newrelic.agent.android.ndk.samples.service.SleepyBackgroundService
import java.util.concurrent.atomic.AtomicLong

class AndroidTestNative : Application() {

    companion object {
        lateinit var alarmProvider: AlarmProvider
        lateinit var serviceIntent: Intent
    }

    override fun onCreate() {
        super.onCreate()
        serviceIntent = Intent(this, SleepyBackgroundService::class.java)
        alarmProvider = AlarmProvider(this)

    }
}