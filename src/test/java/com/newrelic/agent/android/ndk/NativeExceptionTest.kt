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
        Assert.assertTrue(nativeException.message!!.startsWith("SIGILL (code -6)"))
    }

    fun testGetNativeStackFrames() {
        Assert.assertNotNull(nativeException.nativeStackTrace?.stackFrames)
        Assert.assertTrue(nativeException.nativeStackTrace?.stackFrames?.size!! > 1)
        Assert.assertTrue(nativeException.nativeStackTrace!!.stackFrames[0] is StackTraceElement)
    }

    fun testGetStackFrame() {
        Assert.assertNotNull(nativeException.stackTrace)
        Assert.assertTrue(nativeException?.stackTrace.size > 1)
        Assert.assertTrue(nativeException!!.stackTrace[0] is StackTraceElement)
    }

    fun testGetAllThreads() {
        Assert.assertTrue(nativeException.nativeStackTrace!!.threads.size > 1)
    }

    fun testReportsOnlyNativeFrames() {
        val frames = nativeException.stackTrace
        Assert.assertNull(frames.find() { it.lineNumber != -2 })
    }

    fun testFillInStackTrace() {
        var frames = nativeException.fillInStackTrace()
        Assert.assertTrue(frames.stackTrace.size > 1)
    }

}