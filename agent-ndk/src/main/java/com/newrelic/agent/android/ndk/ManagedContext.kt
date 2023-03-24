/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import java.io.File
import java.util.concurrent.TimeUnit

class ManagedContext(val context: Context? = null) {

    var sessionId: String? = null
    var buildId: String? = null
    var reportsDir: File? = getNativeReportsDir(context?.cacheDir)
    var nativeReportListener: AgentNDKListener? = null
    var anrMonitor: Boolean = false
    var expirationPeriod = DEFAULT_TTL

    fun getNativeReportsDir(rootDir: File?): File {
        return File("${rootDir?.absolutePath}/newrelic/reports")
    }

    fun getNativeLibraryDir(context: Context?): File {
        val packageName = context?.packageName
        val packageManager = context?.packageManager
        context?.apply {
            val ainfo: ApplicationInfo? = packageManager?.getApplicationInfo(
                packageName, PackageManager.GET_SHARED_LIBRARY_FILES
            )
            ainfo?.nativeLibraryDir?.apply {
                return File(this)
            }
        }

        return File("./")
    }

    companion object {
        val DEFAULT_TTL = TimeUnit.SECONDS.convert(7, TimeUnit.DAYS)
    }

}