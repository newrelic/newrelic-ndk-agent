package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.crash.Crash
import junit.framework.Assert
import junit.framework.TestCase

class NativeCrashTest : TestCase() {

    val backtrace = this::class.java.classLoader.getResource("backtrace.json").readText()
    lateinit var exception: NativeException
    lateinit var nativeCrash: NativeCrash

    public override fun setUp() {
        exception = NativeException(backtrace)
        nativeCrash = NativeCrash(exception)
    }

    fun testAaJsonObject() {
        val jobj = nativeCrash.asJsonObject()
        Assert.assertTrue(jobj.has("native"))
        Assert.assertNotNull(jobj.get("native"))
    }

    fun testExtractNativeThreads() {
        val threads = nativeCrash.extractNativeThreads(exception)
        Assert.assertFalse(threads.isEmpty())
    }

    fun testExtractThreads() {
        val threads = nativeCrash.extractThreads(exception)
        Assert.assertFalse(threads.isEmpty())
    }

    fun testToString() {
        Assert.assertNotNull(nativeCrash.toString())
    }

    fun testGetNativeException() {
        Assert.assertNotNull(nativeCrash.nativeException)
    }

    fun testToJsonString() {
        val jstr = nativeCrash.toJsonString()
        Assert.assertNotNull(jstr)
        val clonedCrash = Crash.crashFromJsonString(jstr)
        val jobj = clonedCrash.asJsonObject()
        Assert.assertNotNull(jobj)
        // Assert.assertTrue(jobj.has("native"))
    }

}