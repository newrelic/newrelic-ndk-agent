package com.newrelic.agent.android.ndk

import junit.framework.Assert
import junit.framework.TestCase
import java.lang.RuntimeException

class NativeStackTraceTest : TestCase() {
    var nativeCrashStack: NativeStackTrace? = NativeStackTrace()
    val backtrace = this::class.java.classLoader.getResource("backtrace.json").readText()

    public override fun setUp() {
        nativeCrashStack = NativeStackTrace(backtrace)
    }

    fun testFromThrowable() {
        Assert.assertNotNull(NativeStackTrace())
    }

    fun testGetThreads() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertFalse(nativeCrashStack?.threads!!.isEmpty())
        Assert.assertTrue(nativeCrashStack?.threads!!.size > 1)
    }

    fun testGetCrashingThread() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertNotNull(nativeCrashStack?.crashedThread)
        Assert.assertTrue(nativeCrashStack?.crashedThread?.isCrashingThread() == true)
    }

    fun testCrashingThreadHasStacktrace() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertNotNull(nativeCrashStack?.crashedThread)
        Assert.assertNotNull(nativeCrashStack?.crashedThread?.getStackTrace())
        Assert.assertFalse(nativeCrashStack?.crashedThread?.getStackTrace()?.isEmpty() == true)
    }

    fun testGetStackFrames() {
        Assert.assertNotNull(nativeCrashStack?.crashedThread)
        Assert.assertNotNull(nativeCrashStack?.crashedThread?.getStackTrace())
        Assert.assertTrue(nativeCrashStack?.crashedThread?.getStackTrace()?.size!! > 0)
    }

    fun testGetExceptionMessage() {
        Assert.assertNotNull(nativeCrashStack?.exceptionMessage)
        Assert.assertTrue(nativeCrashStack?.exceptionMessage!!.startsWith("SIG"))
    }

}