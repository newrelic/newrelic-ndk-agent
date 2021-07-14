/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_AGENT_NDK_H
#define _AGENT_NDK_AGENT_NDK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <jni.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/cdefs.h>
#include <sys/system_properties.h>
#include <sys/types.h>

#pragma once

static const char *TAG = "newrelic";

// Limit backtrace to 100 frames
static const size_t BACKTRACE_FRAMES_MAX = 100;

// Limit backtrace to 64k// Limit backtrace to 64k
static const size_t BACKTRACE_SZ_MAX = 0x10000;

#include <android/log.h>

#define  _LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,   TAG, __VA_ARGS__)
#define  _LOGW(...)  __android_log_print(ANDROID_LOG_WARN,    TAG, __VA_ARGS__)
#define  _LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,   TAG, __VA_ARGS__)
#define  _LOGI(...)  __android_log_print(ANDROID_LOG_INFO,    TAG, __VA_ARGS__)
#define  _LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)

#endif // _AGENT_NDK_AGENT_NDK_H
