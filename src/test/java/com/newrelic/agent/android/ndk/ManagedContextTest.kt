package com.newrelic.agent.android.ndk

import android.app.Application
import junit.framework.Assert
import junit.framework.TestCase

class ManagedContextTest : TestCase() {

    private lateinit var managedContext: ManagedContext

    public override fun setUp() {
        managedContext = ManagedContext()
    }

    fun testGetContext() {
        Assert.assertEquals(managedContext.context, null)
    }

    fun testSetContext() {}

    fun testGetSessionId() {
        Assert.assertNull(managedContext.sessionId)
    }

    fun testSetSessionId() {}

    fun testGetBuildId() {}

    fun testSetBuildId() {}

    fun testGetReportDirectory() {}

    fun testSetReportDirectory() {}

    fun testGetIpc() {}

    fun testSetIpc() {}

    fun testGetNdkListener() {}

    fun testSetNdkListener() {}
}