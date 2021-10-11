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
import androidx.test.core.app.ApplicationProvider
import com.newrelic.agent.android.api.common.CarrierType
import org.junit.Assert
import org.mockito.Mockito

class SpyContext {
    private val context = ApplicationProvider.getApplicationContext<Context>()
    private var contextSpy = Mockito.spy(context)
    private var packageManager = ApplicationProvider.getApplicationContext<Context>().packageManager
    fun provideSpyContext() {
        contextSpy = Mockito.spy(context.applicationContext)
        packageManager = Mockito.spy(context.packageManager)
        Mockito.`when`(contextSpy.packageManager).thenReturn(packageManager)
        provideActivityManagers(contextSpy)
        val packageInfo = providePackageInfo(contextSpy)
        val applicationInfo = provideApplicationInfo(contextSpy)
        try {
            Mockito.`when`(packageManager.getPackageInfo(contextSpy.packageName, 0))
                .thenReturn(packageInfo)
            Mockito.`when`(packageManager.getApplicationInfo(contextSpy.packageName, 0))
                .thenReturn(applicationInfo)
        } catch (e: PackageManager.NameNotFoundException) {
        }
    }

    fun getContext(): Context {
        return contextSpy
    }

    private fun provideActivityManagers(context: Context) {
        val pids = intArrayOf(Process.myPid())
        val memInfo = arrayOfNulls<Debug.MemoryInfo>(2)
        memInfo[0] = Mockito.mock(Debug.MemoryInfo::class.java)
        memInfo[1] = Mockito.mock(Debug.MemoryInfo::class.java)
        for (memoryInfo in memInfo) {
            Mockito.`when`(memoryInfo!!.totalPss).thenReturn(APP_MEMORY)
        }
        val connectivityManager =
            Mockito.spy(context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager)
        val networkInfo = connectivityManager.activeNetworkInfo
        Mockito.`when`(connectivityManager.activeNetworkInfo).thenReturn(networkInfo)
        Mockito.`when`(context.getSystemService(Context.CONNECTIVITY_SERVICE))
            .thenReturn(connectivityManager)
        val telephonyManager =
            Mockito.spy(context.getSystemService(Context.TELEPHONY_SERVICE) as TelephonyManager)
        Mockito.`when`(telephonyManager.networkOperatorName).thenReturn(CarrierType.WIFI)
        Mockito.`when`(context.getSystemService(Context.TELEPHONY_SERVICE))
            .thenReturn(telephonyManager)
        val activityManager =
            Mockito.spy(context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager)
        Mockito.`when`(activityManager.getProcessMemoryInfo(pids)).thenReturn(memInfo)
        Mockito.`when`(context.getSystemService(Context.ACTIVITY_SERVICE))
            .thenReturn(activityManager)
    }

    private fun providePackageInfo(context: Context): PackageInfo? {
        val packageManager = context.packageManager
        var packageInfo: PackageInfo? = null
        try {
            packageInfo = packageManager.getPackageInfo(context.packageName, 0)
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