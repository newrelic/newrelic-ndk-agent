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
        stackframe?.let { nativeStackFrame.fromJson(it) }
        val element = nativeStackFrame.asStackTraceElement()
        Assert.assertEquals(element.className, "0xee18")
        Assert.assertEquals(element.methodName, "crashBySignal(int)+136")
        Assert.assertTrue(element.fileName.startsWith("/data/app/~~"))
        Assert.assertEquals(element.lineNumber, -2)
    }

    fun testFromJsonObj() {
        nativeStackFrame.fromJson(JSONObject(stackframe!!))
        val element = nativeStackFrame.asStackTraceElement()
        Assert.assertEquals(element.className, "0xee18")
        Assert.assertEquals(element.methodName, "crashBySignal(int)+136")
        Assert.assertTrue(element.fileName.startsWith("/data/app/~~"))
        Assert.assertEquals(element.lineNumber, -2)
    }

    fun testFromJsonExposesModuleOffsets() {
        nativeStackFrame.fromJson(stackframe!!)
        Assert.assertEquals(0x75450c88ee18, nativeStackFrame.address)
        Assert.assertEquals(0xee18L, nativeStackFrame.pc)
        Assert.assertEquals(0x75450c880000, nativeStackFrame.soBase)
        Assert.assertEquals(136L, nativeStackFrame.symAddrOffset)
    }

    fun testFromJsonExposesBuildId() {
        nativeStackFrame.fromJson(stackframe!!)
        Assert.assertEquals("5a1b981dacc1ec9dbd70a074a87b5195c6280545", nativeStackFrame.buildId)
    }

    fun testFromJsonReportsBuildIdWithLibraryPath() {
        // the build id is needed to find the matching symbols, and the stack trace element has
        // no other place to carry it
        val element = nativeStackFrame.fromJson(stackframe!!).asStackTraceElement()
        Assert.assertTrue(element.fileName.startsWith("/data/app/~~"))
        Assert.assertTrue(
            element.fileName.endsWith(
                "libnative-lib.so (BuildId: 5a1b981dacc1ec9dbd70a074a87b5195c6280545)"
            )
        )
    }

    fun testFromJsonWithoutBuildIdLeavesLibraryPathUnchanged() {
        val frame = JSONObject(
            """{"index":1,"address":100,"pc":4096,"so_base":0,"so_path":"/system/lib64/libc.so"}"""
        )
        val nativeFrame = nativeStackFrame.fromJson(frame)
        Assert.assertEquals("", nativeFrame.buildId)
        Assert.assertEquals("/system/lib64/libc.so", nativeFrame.asStackTraceElement().fileName)
    }

    fun testFromJsonReportsModuleRelativePcNotAbsoluteAddress() {
        // frame 0 of the sample app crash: the absolute address changes on every run (ASLR),
        // only the module-relative pc and symbol offset can be symbolicated
        val frame = JSONObject(
            """{"index":0,"address":515369768840,"pc":1928,"so_base":515369766912,
                "sym_addr":515369768808,"sym_addr_offset":32,
                "so_path":"/data/app/base.apk!/lib/arm64-v8a/libsampleapp2.so",
                "sym_name":"Java_com_example_sampleapp2_DoCrash_segFault"}"""
        )
        val element = nativeStackFrame.fromJson(frame).asStackTraceElement()
        Assert.assertEquals("0x788", element.className)
        Assert.assertEquals("Java_com_example_sampleapp2_DoCrash_segFault+32", element.methodName)
        Assert.assertTrue(element.fileName.endsWith("libsampleapp2.so"))
    }

    fun testFromJsonWithoutSymbolOmitsOffset() {
        val frame = JSONObject(
            """{"index":1,"address":100,"pc":4096,"so_base":0,"sym_addr":0,"sym_addr_offset":0,
                "so_path":"/apex/com.android.art/lib64/libart.so"}"""
        )
        val element = nativeStackFrame.fromJson(frame).asStackTraceElement()
        Assert.assertEquals("0x1000", element.className)
        Assert.assertEquals("???", element.methodName)
    }

    fun testFromJsonKeepsZeroPcWhenLibraryWasResolved() {
        // pc is legitimately 0 for the very start of a library; only an unresolved frame
        // (no library base) should fall back to the absolute address
        val frame = JSONObject("""{"index":3,"address":4096,"pc":0,"so_base":4096}""")
        val element = nativeStackFrame.fromJson(frame).asStackTraceElement()
        Assert.assertEquals("0x0", element.className)
    }

    fun testFromJsonFallsBackToAddressWhenPcUnresolved() {
        // dladdr failed, so only the absolute address is known
        val frame = JSONObject("""{"index":2,"address":2048,"pc":0,"so_base":0}""")
        val element = nativeStackFrame.fromJson(frame).asStackTraceElement()
        Assert.assertEquals("0x800", element.className)
    }

    fun testAllFrames() {
        val allFrames = NativeStackTrace(backtrace).crashedThread?.getStackTrace()
        Assert.assertTrue(allFrames?.size!! > 0)
        Assert.assertTrue(allFrames.get(0) is StackTraceElement)
    }

    fun testAllNativeFrames() {
        val stackFrames = JSONObject(backtrace!!)
            .getJSONObject("backtrace")
            .getJSONArray("threads")
            .getJSONObject(0)
            .getJSONArray("stack")
        val allFrames = NativeStackFrame.allNativeFrames(stackFrames)
        Assert.assertFalse(allFrames.isEmpty())
    }
}