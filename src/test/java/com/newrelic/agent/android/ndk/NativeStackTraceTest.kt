package com.newrelic.agent.android.ndk

import junit.framework.Assert
import junit.framework.TestCase
import java.lang.RuntimeException

class NativeStackTraceTest : TestCase() {
    var nativeCrashStack: NativeStackTrace? = NativeStackTrace(RuntimeException())
    val backtrace = this::class.java.classLoader.getResource("backtrace.json").readText()

    public override fun setUp() {
        nativeCrashStack = NativeStackTrace(backtrace)
    }

    fun testFromThrowable() {
        Assert.assertNotNull(NativeStackTrace(NativeException()))
    }

    fun testGetThreads() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertFalse(nativeCrashStack?.threads!!.isEmpty())
        Assert.assertEquals(20, nativeCrashStack?.threads!!.size)
    }

    fun testGetCrashingThread() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertEquals(23802, nativeCrashStack?.crashedThreadId!!)
    }

    fun testGetStackFrames() {
        Assert.assertNotNull(nativeCrashStack?.stackFrames)
        Assert.assertFalse(nativeCrashStack?.stackFrames!!.isEmpty())
        Assert.assertEquals(8, nativeCrashStack?.stackFrames!!.size)
    }

    fun testGetExceptionMessage() {
        Assert.assertNotNull(nativeCrashStack?.exceptionMessage)
        Assert.assertEquals("Native exception", nativeCrashStack?.exceptionMessage)

    }

}