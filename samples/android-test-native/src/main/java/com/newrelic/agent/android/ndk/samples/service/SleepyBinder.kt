/*
 * Copyright (c) 2022-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.newrelic.agent.android.ndk.samples.service

import android.os.IBinder
import android.os.IInterface
import android.os.Parcel
import com.newrelic.agent.android.NewRelic
import java.io.FileDescriptor

class SleepyBinder : IBinder {
    class Interface : IInterface {
        override fun asBinder(): IBinder {
            NewRelic.recordBreadcrumb("asBinder")
            return SleepyBinder()
        }
    }

    override fun getInterfaceDescriptor(): String? {
        NewRelic.recordBreadcrumb("getInterfaceDescriptor")
        return "TODO"
    }

    override fun pingBinder(): Boolean {
        NewRelic.recordBreadcrumb("pingBinder")
        return true
    }

    override fun isBinderAlive(): Boolean {
        NewRelic.recordBreadcrumb("isBinderAlive")
        return true
    }

    override fun queryLocalInterface(descriptor: String): IInterface? {
        NewRelic.recordBreadcrumb("queryLocalInterface")
        return null
    }

    override fun dump(fd: FileDescriptor, args: Array<out String>?) {
        NewRelic.recordBreadcrumb("dump")
    }

    override fun dumpAsync(fd: FileDescriptor, args: Array<out String>?) {
        NewRelic.recordBreadcrumb("dumpAsync")
    }

    override fun transact(code: Int, data: Parcel, reply: Parcel?, flags: Int): Boolean {
        NewRelic.recordBreadcrumb("transact")
        return true
    }

    override fun linkToDeath(recipient: IBinder.DeathRecipient, flags: Int) {
        NewRelic.recordBreadcrumb("linkToDeath")
    }

    override fun unlinkToDeath(recipient: IBinder.DeathRecipient, flags: Int): Boolean {
        NewRelic.recordBreadcrumb("unlinkToDeath")
        return true
    }
}