package com.newrelic.agent.android.ndk

import junit.framework.TestCase

class NativeCrashTest : TestCase() {

    val backtrace = this::class.java.classLoader?.getResource("backtrace.json")?.readText()
    lateinit var exception: NativeException
    lateinit var nativeCrash: NativeCrash

    public override fun setUp() {
        exception = NativeException(backtrace)
        nativeCrash = NativeCrash(exception)
    }

    fun testExtractNativeThreads() {
        val threads = nativeCrash.extractNativeThreads(exception)
        assertFalse(threads.isEmpty())
    }

    fun testExtractThreads() {
        val threads = nativeCrash.extractThreads(exception)
        assertFalse(threads.isEmpty())
    }

    fun testToString() {
        assertNotNull(nativeCrash.toString())
    }

    fun testGetNativeException() {
        assertNotNull(nativeCrash.nativeException)
    }

    fun testToJsonString() {
        val jstr = nativeCrash.toJsonString()
        assertNotNull(jstr)
    }

}