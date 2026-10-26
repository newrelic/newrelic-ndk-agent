package com.newrelic.agent.android.ndk

import junit.framework.TestCase
import org.junit.Assert

class NativeStackTraceTest : TestCase() {
    var nativeCrashStack: NativeStackTrace? = NativeStackTrace()
    val backtrace = this::class.java.classLoader?.getResource("backtrace.json")?.readText()

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

    private fun exceptionMessageFor(
        signalName: String,
        signalCode: Int,
        cause: String,
        faultAddress: Long,
        signalInfoExtras: String = ""
    ): String? {
        val json = """{"backtrace":{"exception":{"name":"Native exception","cause":"$cause",
            "signalInfo":{"signalName":"$signalName","signalCode":$signalCode,
            "faultAddress":$faultAddress$signalInfoExtras}}}}"""
        return NativeStackTrace(json).exceptionMessage
    }

    fun testFaultSignalReportsFaultAddress() {
        Assert.assertEquals(
            "SIGSEGV (code 1) Address not mapped to object at 0xdeadbeef",
            exceptionMessageFor("SIGSEGV", 1, "Address not mapped to object", 0xdeadbeefL)
        )
    }

    fun testUserSentSignalReportsSenderInsteadOfFaultAddress() {
        // abort() sends SIGABRT with si_code SI_QUEUE (-1); si_addr overlaps si_pid/si_uid, so
        // faultAddress (uid << 32 | pid = 0x27fb0000158f) is not an address and must not be
        // reported as one
        val message = exceptionMessageFor(
            "SIGABRT", -1, "SIGABRT", 43958990280079L, ""","senderPid":5519,"senderUid":10235"""
        )
        Assert.assertEquals("SIGABRT (code -1) sent by pid 5519 (uid 10235)", message)
    }

    fun testUserSentSignalKeepsDistinctCause() {
        val message = exceptionMessageFor(
            "SIGABRT", -6, "SIG_TKILL", 43958990280079L, ""","senderPid":42,"senderUid":1000"""
        )
        Assert.assertEquals("SIGABRT (code -6) SIG_TKILL sent by pid 42 (uid 1000)", message)
    }

    fun testUserSentSignalWithoutSenderOmitsFaultAddress() {
        // reports from agents that don't record the sender
        val message = exceptionMessageFor("SIGABRT", -1, "SIGABRT", 43958990280079L)
        Assert.assertEquals("SIGABRT (code -1)", message)
    }

    fun testGetExceptionMessage() {
        Assert.assertNotNull(nativeCrashStack?.exceptionMessage)
        Assert.assertTrue(nativeCrashStack?.exceptionMessage!!.startsWith("SIG"))
    }

}