package com.newrelic.agent.android.ndk

import android.content.Context
import com.newrelic.agent.android.SpyContext
import junit.framework.Assert
import junit.framework.TestCase
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.File
import java.nio.ByteBuffer
import java.util.concurrent.TimeUnit

@RunWith(RobolectricTestRunner::class)
class ManagedContextTest : TestCase(), AgentNDKListener {

    var context: Context? = null
    var managedContext: ManagedContext? = null

    @Before
    public override fun setUp() {
        context = SpyContext().getContext()
        managedContext = ManagedContext(context)
    }

    @Test
    fun testGetContext() {
        Assert.assertNotNull(managedContext?.context)
    }

    @Test
    fun testSetContext() {
        Assert.assertEquals(context, managedContext?.context)
    }

    @Test
    fun testGetSessionId() {
        Assert.assertNull(managedContext?.sessionId)
    }

    @Test
    fun testGetBuildId() {
        Assert.assertNull(managedContext?.buildId)
    }

    @Test
    fun testGetReportDirectory() {
        val target = File("${context?.cacheDir?.absolutePath}/newrelic/reports")
        Assert.assertEquals(target.absolutePath, managedContext?.reportsDir?.absolutePath)
    }

    @Test
    fun testGetNdkListener() {
        Assert.assertNull(managedContext?.nativeReportListener)
    }

    @Test
    fun testSetNdkListener() {
        managedContext?.nativeReportListener = this
        Assert.assertNotNull(managedContext?.nativeReportListener)
        Assert.assertEquals(this, managedContext?.nativeReportListener)
    }

    @Test
    fun testGetANRMonitor() {
        Assert.assertFalse(managedContext?.anrMonitor == true)
    }

    @Test
    fun testExpirationPeriod() {
        Assert.assertEquals(managedContext?.expirationPeriod, ManagedContext.DEFAULT_TTL)
        Assert.assertEquals(managedContext?.expirationPeriod, TimeUnit.SECONDS.convert(7, TimeUnit.DAYS))
    }

    override fun onNativeCrash(crashAsString: String?): Boolean {
        TODO("Not yet implemented")
    }

    override fun onNativeException(exceptionAsString: String?): Boolean {
        TODO("Not yet implemented")
    }

    override fun onApplicationNotResponding(anrAsString: String?): Boolean {
        TODO("Not yet implemented")
    }
}