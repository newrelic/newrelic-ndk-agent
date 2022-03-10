package com.newrelic.agent.android.ndk

import android.os.Looper
import org.junit.Assert
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ANRMonitorTest {

    var anrMonitor: ANRMonitor? = null

    @Before
    fun setUp() {
        anrMonitor = ANRMonitor()
    }

    @Test
    fun ignoreWhileDebugging() {
        Assert.assertFalse(anrMonitor?.disableWhileDebugging == true)
        anrMonitor?.disableWhileDebugging()
        Assert.assertFalse(anrMonitor?.disableWhileDebugging == false)
    }

    @Test
    fun testGetHandler() {
        Assert.assertEquals(anrMonitor?.handler?.looper, Looper.getMainLooper())
    }

    @Test
    fun testStartMonitor() {
        Assert.assertNull(anrMonitor?.future)
        anrMonitor?.startMonitor()
        Assert.assertTrue(anrMonitor?.isRunning() == true)
    }

    @Test
    fun testStopMonitor() {
        anrMonitor?.startMonitor()
        Assert.assertTrue(anrMonitor?.isRunning() == true)
        anrMonitor?.stopMonitor()
        Assert.assertFalse(anrMonitor?.isRunning() == true)
    }

    @Test
    fun testExecutor() {
        Assert.assertFalse(anrMonitor?.executor?.isShutdown == true)
    }

}