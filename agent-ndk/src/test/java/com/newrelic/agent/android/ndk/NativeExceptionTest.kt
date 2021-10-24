package com.newrelic.agent.android.ndk

import junit.framework.Assert
import junit.framework.TestCase

class NativeExceptionTest : TestCase() {

    val backtrace = this::class.java.classLoader.getResource("backtrace.json").readText()

    lateinit var nativeException: NativeException

    public override fun setUp() {
        nativeException = NativeException(backtrace)
    }

    fun testGetNativeStackTrace() {
        Assert.assertNotNull(nativeException.nativeStackTrace)
    }

    fun testGetMessage() {
        Assert.assertEquals("SIGILL (code -6) Illegal operation at 0x1f2d", nativeException.message)
    }

    fun testGetNativeStackFrames() {
        Assert.assertNotNull(nativeException.nativeStackTrace.stackFrames)
        Assert.assertEquals(5, nativeException.nativeStackTrace.stackFrames.size)
        Assert.assertTrue(nativeException.nativeStackTrace.stackFrames[0] is StackTraceElement)
    }

    fun testGetStackFrame() {
        Assert.assertNotNull(nativeException.stackTrace)
        Assert.assertEquals(5, nativeException.stackTrace.size)
        Assert.assertTrue(nativeException.stackTrace[0] is StackTraceElement)
    }

    fun testGetAllThreads() {
        Assert.assertEquals(20, nativeException.nativeStackTrace.threads.size)
    }

    fun testGetCrashedThreadId() {
        Assert.assertEquals(8058, nativeException.nativeStackTrace.crashedThreadId)
    }

    fun testReportsOnlyNativeFrames() {
        val frames = nativeException.stackTrace
        Assert.assertNull(frames.find() { it.lineNumber != -2 })
    }

    fun testFillInStackTrace() {
        var frames = nativeException.fillInStackTrace()
        Assert.assertEquals(5, frames.stackTrace.size)
    }

}