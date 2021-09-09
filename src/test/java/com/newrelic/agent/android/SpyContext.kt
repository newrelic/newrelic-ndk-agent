package com.newrelic.agent.android

import android.app.ActivityManager
import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.os.Debug
import android.os.Process
import android.telephony.TelephonyManager
import com.newrelic.agent.android.api.common.CarrierType
import org.junit.Assert
import org.mockito.Mockito
import org.robolectric.RuntimeEnvironment

class SpyContext {
    var context: Context = RuntimeEnvironment.application
        private set
    private var packageManager = RuntimeEnvironment.application.packageManager
    fun provideSpyContext(): SpyContext {
        packageManager = Mockito.spy(RuntimeEnvironment.application.packageManager)
        context = Mockito.spy(RuntimeEnvironment.application)
        Mockito.`when`(context.applicationContext).thenReturn(RuntimeEnvironment.application)
        Mockito.`when`(context.packageManager).thenReturn(packageManager)
        provideActivityManagers(context)
        val packageInfo = providePackageInfo(context)
        val applicationInfo = provideApplicationInfo(context)
        try {
            Mockito.`when`(packageManager.getPackageInfo(context.packageName, 0)).thenReturn(packageInfo)
            Mockito.`when`(packageManager.getApplicationInfo(context.packageName, 0)).thenReturn(applicationInfo)
        } catch (e: PackageManager.NameNotFoundException) {
        }
        return this
    }

    fun provideActivityManagers(context: Context) {
        val pids = intArrayOf(Process.myPid())
        val memInfo = arrayOfNulls<Debug.MemoryInfo>(2)
        memInfo[0] = Mockito.mock(Debug.MemoryInfo::class.java)
        memInfo[1] = Mockito.mock(Debug.MemoryInfo::class.java)
        for (memoryInfo in memInfo) {
            Mockito.`when`(memoryInfo!!.totalPss).thenReturn(APP_MEMORY)
        }
        val connectivityManager = Mockito.spy(context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager)
        val networkInfo = connectivityManager.activeNetworkInfo
        Mockito.`when`(connectivityManager.activeNetworkInfo).thenReturn(networkInfo)
        Mockito.`when`(context.getSystemService(Context.CONNECTIVITY_SERVICE)).thenReturn(connectivityManager)
        val telephonyManager = Mockito.spy(context.getSystemService(Context.TELEPHONY_SERVICE) as TelephonyManager)
        Mockito.`when`(telephonyManager.networkOperatorName).thenReturn(CarrierType.WIFI)
        Mockito.`when`(context.getSystemService(Context.TELEPHONY_SERVICE)).thenReturn(telephonyManager)
        val activityManager = Mockito.spy(context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager)
        Mockito.`when`(activityManager.getProcessMemoryInfo(pids)).thenReturn(memInfo)
        Mockito.`when`(context.getSystemService(Context.ACTIVITY_SERVICE)).thenReturn(activityManager)
    }

    fun providePackageInfo(context: Context): PackageInfo? {
        val packageManager = context.packageManager
        var packageInfo: PackageInfo? = null
        try {
            packageInfo = Mockito.spy(packageManager.getPackageInfo(context.packageName, 0))
            packageInfo.versionName = APP_VERSION_NAME
            packageInfo.versionCode = APP_VERSION_CODE
        } catch (e: PackageManager.NameNotFoundException) {
            Assert.fail()
        }
        return packageInfo
    }

    private fun provideApplicationInfo(context: Context): ApplicationInfo? {
        val packageManager = context.packageManager
        var applicationInfo: ApplicationInfo? = null
        try {
            applicationInfo = Mockito.spy(packageManager.getApplicationInfo(context.packageName, 0))
            applicationInfo.name = javaClass.simpleName
        } catch (e: PackageManager.NameNotFoundException) {
            Assert.fail()
        }
        return applicationInfo
    }

    companion object {
        const val APP_VERSION_NAME = "1.1"
        const val APP_VERSION_CODE = 99
        const val APP_MEMORY = 0xbadf00d
    }

    init {
        provideSpyContext()
    }
}