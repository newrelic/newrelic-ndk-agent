/*
 * Copyright (c) 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk

import junit.framework.TestCase
import org.json.JSONObject
import org.junit.Assert

class NativeStackFrameTest : TestCase() {

    val stackframe = this::class.java.classLoader?.getResource("stackframe.json")?.readText()
    val backtrace = this::class.java.classLoader?.getResource("backtrace.json")?.readText()
    lateinit var nativeStackFrame: NativeStackFrame

    public override fun setUp() {
        nativeStackFrame = NativeStackFrame("class", "method", "file", -99)
    }

    fun testGetDelegate() {
        Assert.assertEquals(nativeStackFrame.delegate.className, "class")
        Assert.assertEquals(nativeStackFrame.delegate.methodName, "method")
        Assert.assertEquals(nativeStackFrame.delegate.fileName, "file")
        Assert.assertEquals(nativeStackFrame.delegate.lineNumber, -99)
    }

    fun testSetDelegate() {
        nativeStackFrame.delegate = StackTraceElement("a", "b", "c", -99)
        Assert.assertEquals(nativeStackFrame.delegate.className, "a")
        Assert.assertEquals(nativeStackFrame.delegate.methodName, "b")
        Assert.assertEquals(nativeStackFrame.delegate.fileName, "c")
        Assert.assertEquals(nativeStackFrame.delegate.lineNumber, -99)
    }

    fun testAsStackTraceElement() {
        val element = nativeStackFrame.asStackTraceElement()
        Assert.assertEquals(element.className, "class")
        Assert.assertEquals(element.methodName, "method")
        Assert.assertEquals(element.fileName, "file")
        Assert.assertEquals(element.lineNumber, -99)
    }

    fun testFromJson() {
        nativeStackFrame.fromJson(stackframe)
        val element = nativeStackFrame.asStackTraceElement()
        Assert.assertEquals(element.className, "")
        Assert.assertEquals(element.methodName, "crashBySignal(int)")
        Assert.assertTrue(element.fileName.startsWith("/data/app/~~"))
        Assert.assertEquals(element.lineNumber, -2)
    }

    fun testFromJsonObj() {
        nativeStackFrame.fromJson(JSONObject(stackframe))
        val element = nativeStackFrame.asStackTraceElement()
        Assert.assertEquals(element.className, "")
        Assert.assertEquals(element.methodName, "crashBySignal(int)")
        Assert.assertTrue(element.fileName.startsWith("/data/app/~~"))
        Assert.assertEquals(element.lineNumber, -2)
    }

    fun testAllFrames() {
        val allFrames = NativeStackTrace(backtrace).crashedThread?.getStackTrace()
        Assert.assertTrue(allFrames?.size!! > 0)
        Assert.assertTrue(allFrames.get(0) is StackTraceElement)
    }

    fun testAllNativeFrames() {
        val stackFrames = JSONObject(backtrace)
            .getJSONObject("backtrace")
            .getJSONArray("threads")
            .getJSONObject(0)
            .getJSONArray("stack")
        val allFrames = NativeStackFrame.allNativeFrames(stackFrames)
        Assert.assertFalse(allFrames.isEmpty())
    }
}