package com.newrelic.agent.android.ndk

import junit.framework.Assert
import junit.framework.TestCase
import com.newrelic.agent.android.ndk.NativeStackFrame as NativeStackFrame

class NativeExceptionTest : TestCase() {

    val backtrace = this::class.java.classLoader.getResource("crash.json").readText()

    lateinit var nativeException: NativeException

    public override fun setUp() {
        nativeException = NativeException(backtrace)
    }

    fun testGetNativeStackTrace() {
        Assert.assertNotNull(nativeException.nativeStackTrace)
    }

    fun testGetMessage() {
        Assert.assertEquals(nativeException.message, "Native exception")
    }

    fun testGetNativeStackFrames() {
        Assert.assertNotNull(nativeException.nativeStackTrace.stackFrames)
        Assert.assertEquals(75, nativeException.nativeStackTrace.stackFrames.size)
        Assert.assertTrue(nativeException.nativeStackTrace.stackFrames[0] is StackTraceElement)
    }

    fun testGetStackFrame() {
        Assert.assertNotNull(nativeException.stackTrace)
        Assert.assertEquals(75, nativeException.stackTrace.size)
        Assert.assertTrue(nativeException.stackTrace[0] is StackTraceElement)
    }

    fun testGetCrashedThreadId() {
        Assert.assertEquals(9555, nativeException.nativeStackTrace.crashedThreadId)
    }
}