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
        Assert.assertTrue(nativeCrashStack?.threads!!.size > 1)
    }

    fun testGetCrashingThread() {
        Assert.assertNotNull(nativeCrashStack?.threads)
        Assert.assertTrue(nativeCrashStack?.crashedThreadId!! > 0)
    }

    fun testGetStackFrames() {
        Assert.assertNotNull(nativeCrashStack?.stackFrames)
        Assert.assertFalse(nativeCrashStack?.stackFrames!!.isEmpty())
        Assert.assertTrue(nativeCrashStack?.stackFrames!!.size > 1)
    }

    fun testGetExceptionMessage() {
        Assert.assertNotNull(nativeCrashStack?.exceptionMessage)
        Assert.assertTrue(nativeCrashStack?.exceptionMessage!!.startsWith("SIGILL (code -6)"))
    }

}