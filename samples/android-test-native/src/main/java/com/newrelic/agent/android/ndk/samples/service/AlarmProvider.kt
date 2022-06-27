package com.newrelic.agent.android.ndk.samples.service

import android.app.AlarmManager
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.SystemClock
import android.util.Log
import com.newrelic.agent.android.NewRelic
import java.util.*

/**
 * @link https://developer.android.com/training/scheduling/alarms
 */
class AlarmProvider(context: Context) {
    private var context: Context = context
    private var alarmIntent: PendingIntent
    private var alarmManager: AlarmManager? = null

    class AlarmReceiver : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            Log.i(SleepyBackgroundService.TAG, "Alarm received")
            var attrs = HashMap<String, Any>().apply {
                intent?.apply {
                    action?.let { it -> put("action", it) }
                    put("flags", flags)
                    categories?.joinToString()?.let { it1 -> put("category", it1) }
                }
            }
            NewRelic.recordBreadcrumb("AlarmReceiver", attrs)
            Thread.sleep(12000)     // generate ANR?
        }
    }

    companion object {
        private val ACTION: String = "android.intent.action.BOOT_COMPLETED"; // "com.newrelic.android.test.bgm.ACTION"
        private val INTERVAL_3_MIN: Long = (3 * 60 * 1000)
        private val SNOOZE_5_MIN: Long = (5 * 60 * 1000)

    }

    // alarm for 8:30am
    val morningAlarm: Calendar = Calendar.getInstance().apply {
        timeInMillis = System.currentTimeMillis()
        set(Calendar.HOUR_OF_DAY, 8)
        set(Calendar.MINUTE, 30)
    }

    init {
        alarmManager = context.getSystemService(Context.ALARM_SERVICE) as? AlarmManager
        alarmIntent = Intent(context, AlarmReceiver::class.java).let { intent ->
            PendingIntent.getBroadcast(context, 0, intent, 0)
        }

        val requestId = 0
        val serviceIntent = Intent(context, SleepyBackgroundService::class.java)
        val pendingIntent = PendingIntent.getService(context, requestId, serviceIntent, PendingIntent.FLAG_NO_CREATE)

        if (pendingIntent != null && alarmManager != null) {
        //  alarmManager?.cancel(pendingIntent)
        }
    }

    /**
     * RTC_WAKEUP: Wakes up the device to fire the pending intent at the specified time.
     * RTC: Fires the pending intent at the specified time but does not wake up the device.
     *
     * Wake the device at 8:30'ish then again every 10 minutes. Since this is a
     * system (headless) process, the agent must be started before any work is done.
     * It's safe to call Agent.start() on an active agent.
     */
    fun setWakeyWakey() {
        // SleepyBackgroundService.INSTANCE?.startNewRelicAgent();

        alarmManager?.setRepeating(
                AlarmManager.RTC_WAKEUP, morningAlarm.timeInMillis, SNOOZE_5_MIN, alarmIntent)
    }

    fun setRTCAlarm(delay: Long) {
        var rtcIntent = Intent(context, AlarmReceiver::class.java).let { intent ->
            PendingIntent.getBroadcast(context, 0, intent, 0)
        }

        alarmManager?.set(
                AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delay,
                rtcIntent
        )
    }

    /**
     * ELAPSED_REALTIME_WAKEUP: Wakes up the device and fires the pending intent
     * after the specified length of time has elapsed since device boot.
     */
    fun setInexact3MinRepeatingAlarm(duration: Long) {
        var repeatingIntent = Intent(context, AlarmReceiver::class.java).let { intent ->
            PendingIntent.getBroadcast(context, 0, intent, 0)
        }

        alarmManager?.setInexactRepeating(
                AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + INTERVAL_3_MIN,
                duration,
                repeatingIntent)
    }

    /**
     * To cancel a PendingIntent, pass FLAG_NO_CREATE to PendingIntent.getService() to get
     * an instance of the intent (if it exists), then pass that intent to AlarmManager.cancel():
     */
    fun cancel(intent: PendingIntent) {
        alarmManager?.cancel(intent)
    }

    /**
     * To prevent the boot receiver from being called unnecessarily, disable the alarm
     * in the manifest call this to enable it at runtime. Once the alarm is enabled,
     * it stays enabled, even across reboots of the device.
     */
    fun enableAlarm() {
        val receiver = ComponentName(context, AlarmReceiver::class.java)
        context.packageManager.setComponentEnabledSetting(
                receiver,
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP
        )
    }

    /**
     * Enabling the receiver overrides the manifest setting, even across reboots.
     * The receiver will stay enabled until it is manually disabled.
     */
    fun disableAlarm() {
        val receiver = ComponentName(context, AlarmReceiver::class.java)
        context.packageManager.setComponentEnabledSetting(
                receiver,
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP
        )
    }
}

