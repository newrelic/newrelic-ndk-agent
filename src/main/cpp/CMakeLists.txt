##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

cmake_minimum_required(VERSION 3.18.1)

project("agent-ndk")   ## VERSION ${AGENT_VERSION} LANGUAGES CXX)

include(FetchContent)

if (0)
    # abi's don't have JNI?
    find_package(JNI REQUIRED)
    if (JNI_FOUND)
        message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
        message(STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
        include_directories(${JAVA_INCLUDE_PATH})
    else ()
        message(STATUS "JNI NOT FOUND")
    endif ()
endif ()

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden)

set(PROJECT_TEST_DIR ${CMAKE_HOME_DIRECTORY}/../../test/cpp)
message(STATUS "${PROJECT_TEST_DIR}")

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
        jni/native_context.cpp
        serializer.cpp
        backtrace.cpp
        )

target_include_directories(agent-ndk PUBLIC include)
target_include_directories(agent-ndk PRIVATE .)


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

# NDK support libs: https://developer.android.com/ndk/guides/stable_apis
find_library(log-lib log)

target_link_libraries(agent-ndk ${log-lib})

##
# For debugging
##
add_library(agent-ndk-a STATIC
        agent-ndk.cpp
        signal-handler.cpp
        terminate-handler.cpp
        anr-handler.cpp
        unwinder.cpp
        procfs.cpp
        signal-utils.cpp
        jni/jni.cpp
        jni/jni-delegate.cpp
        jni/native_context.cpp
        serializer.cpp
        backtrace.cpp
        )

target_include_directories(agent-ndk-a PUBLIC include)
target_include_directories(agent-ndk PRIVATE .)
target_link_libraries(agent-ndk-a ${log-lib})

# Include testing framework(s) and test app
## FIXME include(UnitTests.cmake)
