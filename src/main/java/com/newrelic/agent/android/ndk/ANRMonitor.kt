/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.os.Handler
import android.os.Looper

open class ANRMonitor {

    private val DEFAULT_TIMEOUT = 5000
    private val anrHandler: Handler = Handler(Looper.getMainLooper())
    private var ignoreWhileDebugging = false

    interface ANRListener {
        /**
         * Notification that an ANR event has occurred.
         */
        fun onANRDetected(anrStackTrace: ANRStackTrace): Void
    }

    /**
     * Ignore detected ANRs when attached to a debugger
     */
    fun ignoreWhileDebugging() {
        ignoreWhileDebugging = true
    }

    companion object {
        init {

        }
    }
}