/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */
package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.agentdata.AgentDataController
import com.newrelic.agent.android.analytics.AnalyticsAttribute
import com.newrelic.agent.android.analytics.AnalyticsControllerImpl
import com.newrelic.agent.android.crash.Crash
import com.newrelic.agent.android.crash.CrashReporter
import com.newrelic.agent.android.logging.AgentLogManager
import java.util.*

class JVMDelegate() : AgentNDKListener {

    /**
     * Delegated classes:
     * com.newrelic.agent.android.crash.Crash
     * com.newrelic.agent.android.agentdata.AgentDataController
     */

    /**
     * A native crash has been detected and forwarded to this method
     */
    override fun onNativeCrash(crashAsString: String?) {
        val crash = Crash(
            Throwable(),
            analyticsController.sessionAttributes,
            analyticsController.eventManager.queuedEvents,
            true
        )
        CrashReporter.getInstance().storeAndReportCrash(crash)
    }

    /**
     * A native runtime exception has been detected and forwarded to this method
     */
    override fun onNativeException(exceptionAsString: String?) {
        val exceptionAttributes: HashMap<String?, Any?> = object : HashMap<String?, Any?>() {
            init {
                put(AnalyticsAttribute.APPLICATION_PLATFORM_ATTRIBUTE, "native")
            }
        }
        val exceptionToHandle: Exception = RuntimeException("FIXME: native")
        AgentDataController.sendAgentData(exceptionToHandle, exceptionAttributes)
    }

    /**
     * ANR condition has been detected and forwarded to this method
     */
    override fun onApplicationNotResponding(stackTraceAsString: String?) {
        val exceptionAttributes: HashMap<String?, Any?> = object : HashMap<String?, Any?>() {
            init {
                put(AnalyticsAttribute.APPLICATION_PLATFORM_ATTRIBUTE, "native")
                put("ANR", "true")
            }
        }
        val exceptionToHandle: Exception = InterruptedException("Application not responding")
        AgentDataController.sendAgentData(exceptionToHandle, exceptionAttributes)
    }

    companion object {
        private val analyticsController = AnalyticsControllerImpl.getInstance()

        var delegateMethods: HashMap<String?, String?> = object : HashMap<String?, String?>() {
            init {
                put("onNativeCrash", "(Ljava/lang/String;)V")
                put("onNativeException", "(Ljava/lang/String;)V")
                put("onApplicationNotRespondingMethod", "(Ljava/lang/String;)V")
            }
        }

        fun getSignature(methodName: String): String {
            return delegateMethods[methodName] ?: "()V"
        }
    }
}