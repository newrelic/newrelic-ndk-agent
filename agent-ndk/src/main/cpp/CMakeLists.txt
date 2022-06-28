##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

cmake_minimum_required(VERSION 3.18.1)

project("agent-ndk" LANGUAGES CXX)

include(FetchContent)

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden -funwind-tables -fexceptions -frtti)

message(STATUS "Cmake build type is ${CMAKE_BUILD_TYPE}")

add_library(agent-ndk
        SHARED
        agent-ndk.cpp
        signal-handler.cpp
        terminate-handler.cpp
        anr-handler.cpp
        unwinder.cpp
        procfs.cpp
        signal-utils.cpp
        jni/jni.cpp
        jni/jni-delegate.cpp
        jni/native-context.cpp
        serializer.cpp
        backtrace.cpp
        emitter.cpp
        )

find_library(log-lib log)

target_include_directories(agent-ndk PUBLIC include)
target_include_directories(agent-ndk PRIVATE .)
target_link_libraries(agent-ndk ${log-lib})

##
# For tests and debugging
##
add_library(agent-ndk-a
        STATIC
        agent-ndk.cpp
        signal-handler.cpp
        terminate-handler.cpp
        anr-handler.cpp
        unwinder.cpp
        procfs.cpp
        signal-utils.cpp
        jni/jni.cpp
        jni/jni-delegate.cpp
        jni/native-context.cpp
        serializer.cpp
        backtrace.cpp
        emitter.cpp
        )

target_include_directories(agent-ndk-a PUBLIC include)
target_include_directories(agent-ndk-a PRIVATE .)
target_link_libraries(agent-ndk-a ${log-lib})

# Include testing framework(s) and test app
include(UnitTests.cmake)
