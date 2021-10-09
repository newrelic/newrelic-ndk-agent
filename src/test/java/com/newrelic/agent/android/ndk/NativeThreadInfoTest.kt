package com.newrelic.agent.android.ndk

import junit.framework.Assert
import junit.framework.TestCase
import org.json.JSONObject

class NativeThreadInfoTest : TestCase() {

    val backtrace = this::class.java.classLoader.getResource("backtrace.json").readText()
    val threadInfo = this::class.java.classLoader.getResource("threadInfo.json").readText()

    var nativeThreadInfo: NativeThreadInfo = NativeThreadInfo(threadInfo)

    public override fun setUp() {
        super.setUp()
        nativeThreadInfo = NativeThreadInfo(threadInfo)
    }

    fun testFromJsonObject() {
        nativeThreadInfo = NativeThreadInfo().fromJsonObject(JSONObject(threadInfo))
        Assert.assertEquals(12569, nativeThreadInfo.threadId)
    }

    fun testFromJson() {
        nativeThreadInfo = NativeThreadInfo().fromJson(threadInfo)
        Assert.assertEquals(12569, nativeThreadInfo.threadId)
    }

    fun testAllThreads() {
        val threads = JSONObject(backtrace).getJSONObject("backtrace").getJSONArray("threads")
        val allThreads = NativeThreadInfo.allThreads(threads)
        Assert.assertEquals(20, allThreads.size)
    }

    fun testCrashingThread() {
        Assert.assertTrue(nativeThreadInfo.isCrashingThread())
    }
}