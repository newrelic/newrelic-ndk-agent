package com.newrelic.agent.android.ndk

import com.newrelic.agent.android.harvest.crash.ThreadInfo
import java.util.*

class JVMStackTrace(exception: Exception = RuntimeException()) {
    val threads: List<ThreadInfo> = ArrayList()
    var stackTrace: Array<StackTraceElement> = exception.stackTrace
    val crashedThread = ThreadInfo(Thread.currentThread(), exception.stackTrace)
    val crashedThreadId = crashedThread.threadId

    val threadInfo: ThreadInfo = ThreadInfo(exception)
}
