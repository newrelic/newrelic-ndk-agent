/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_AGENT_NDK_H
#define _AGENT_NDK_AGENT_NDK_H

#pragma once

#ifndef AGENT_VERSION
#define AGENT_VERSION "0.5.0"
#endif  // !AGENT_VERSION

static const char *TAG = "com.newrelic.android";

// Limit backtrace to 100 frames
static const size_t BACKTRACE_FRAMES_MAX = 100;

// Limit each frame to 1k
static const size_t BACKTRACE_FRAMES_SZ_MAX = 1024;

// Limit backtrace to 100 threads
static const size_t BACKTRACE_THREADS_MAX = 100;

// Limit backtrace to 1Mb
static const size_t BACKTRACE_SZ_MAX = 0x100000;


/**
 * Return a literal string representing the current architecture
 */
const char *get_arch();

/**
 * Collect and return a complete backtrace report into the provided buffer
 */
bool collect_backtrace(char *, size_t, const siginfo_t *, const ucontext_t *);


#include <android/log.h>

#define  _LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,   TAG, __VA_ARGS__)
#define  _LOGW(...)  __android_log_print(ANDROID_LOG_WARN,    TAG, __VA_ARGS__)
#define  _LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,   TAG, __VA_ARGS__)
#define  _LOGI(...)  __android_log_print(ANDROID_LOG_INFO,    TAG, __VA_ARGS__)

#endif // _AGENT_NDK_AGENT_NDK_H
