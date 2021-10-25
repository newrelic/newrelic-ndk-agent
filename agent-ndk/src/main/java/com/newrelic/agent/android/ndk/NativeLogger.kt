package com.newrelic.agent.android.ndk

import android.util.Log
import com.newrelic.agent.android.logging.ConsoleAgentLog

class NativeLogger : ConsoleAgentLog() {
    val TAG = "newrelic"
    override fun print(tag: String?, message: String?) {
        Log.d(TAG, message)
    }
}