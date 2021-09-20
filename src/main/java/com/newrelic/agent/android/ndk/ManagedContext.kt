/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import java.io.File
import java.nio.ByteBuffer

class ManagedContext(context: Context? = null) {

    var context: Context? = context
    var sessionId: String? = null
    var buildId: String? = null
    var reportsDir: File? = getNativeReportsDir(context?.cacheDir)
    var libDir: File? = getNativeLibraryDir(context)
    var ipc: ByteBuffer = ByteBuffer.allocateDirect(0x10000)
    var nativeReportListener: AgentNDKListener = JVMDelegate()
    var anrMonitor: Boolean = false

    fun getNativeReportsDir(rootDir: File?): File {
        return File("${rootDir?.absolutePath}/newrelic/reports")
    }

    fun getNativeLibraryDir(context: Context?): File {
        val packageName = context?.packageName
        val packageManager = context?.packageManager
        context?.apply {
            val ainfo: ApplicationInfo? = packageManager?.getApplicationInfo(
                packageName, PackageManager.GET_SHARED_LIBRARY_FILES)
            return File(ainfo?.nativeLibraryDir)
        }

        return File("./")
    }

}