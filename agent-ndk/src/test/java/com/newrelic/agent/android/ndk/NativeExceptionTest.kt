package com.newrelic.agent.android.ndk

import junit.framework.TestCase
import org.junit.Assert

class NativeExceptionTest : TestCase() {

    val backtrace = this::class.java.classLoader?.getResource("backtrace.json")?.readText()
    lateinit var nativeException: NativeException

    public override fun setUp() {
        nativeException = NativeException(backtrace)
    }

    fun testGetNativeStackTrace() {
        Assert.assertNotNull(nativeException.nativeStackTrace)
    }

    fun testGetMessage() {
        Assert.assertTrue(nativeException.message!!.startsWith("SIG"))
    }

    fun testGetNativeStackFrames() {
        val stacktrace = nativeException.nativeStackTrace?.crashedThread?.getStackTrace()
        Assert.assertNotNull(stacktrace)
        Assert.assertTrue(stacktrace?.size!! > 0)
        Assert.assertTrue(stacktrace[0] is StackTraceElement)
    }

    fun testGetStackFrame() {
        Assert.assertNotNull(nativeException.stackTrace)
        Assert.assertTrue(nativeException.stackTrace.size > 0)
    }

    fun testGetAllThreads() {
        Assert.assertTrue(nativeException.nativeStackTrace!!.threads.size > 0)
    }

    fun testReportsOnlyNativeFrames() {
        val frames = nativeException.stackTrace
        Assert.assertNull(frames.find() { it.lineNumber != -2 })
    }

    fun testFillInStackTrace() {
        nativeException = NativeException(backtrace)
        var frames = nativeException.fillInStackTrace()
        Assert.assertTrue(frames.stackTrace.size > 0)
    }

}