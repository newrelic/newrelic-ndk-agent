##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

cmake_minimum_required(VERSION 3.18.1)

project("agent-ndk")   ## VERSION ${AGENT_VERSION} LANGUAGES CXX)

include(FetchContent)

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden)

message(STATUS "Cmake build type is ${DCMAKE_BUILD_TYPE}")

set(PROJECT_TEST_DIR ${CMAKE_HOME_DIRECTORY}/../../test/cpp)
# message(STATUS "${PROJECT_TEST_DIR}")

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
        jni/native-context.cpp
        serializer.cpp
        backtrace.cpp
        emitter.cpp
        )

target_include_directories(agent-ndk-a PUBLIC include)
target_include_directories(agent-ndk PRIVATE .)
target_link_libraries(agent-ndk-a ${log-lib})

# Include testing framework(s) and test app
## FIXME include(UnitTests.cmake)

set(GOOGLETEST_ROOT ${ANDROID_NDK}/sources/third_party/googletest/googletest)
# add_library(gtest STATIC ${GOOGLETEST_ROOT}/src/gtest_main.cc ${GOOGLETEST_ROOT}/src/gtest-all.cc)
# target_include_directories(gtest PRIVATE ${GOOGLETEST_ROOT})
# target_include_directories(gtest PUBLIC ${GOOGLETEST_ROOT}/include)

# add_executable(footesttest src/main/jni/foo_unittest.cc)
# target_link_libraries(abi-test gtest)

# find_program(ADB adb)
# add_custom_command(TARGET abi-test POST_BUILD
#        COMMAND ${ADB} shell mkdir -p /data/local/tmp/${ANDROID_ABI}
#        COMMAND ${ADB} push $<TARGET_FILE:native-lib> /data/local/tmp/${ANDROID_ABI}/
#        COMMAND ${ADB} push $<TARGET_FILE:footest> /data/local/tmp/${ANDROID_ABI}/
#        COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=/data/local/tmp/${ANDROID_ABI}\; /data/local/tmp/${ANDROID_ABI}/footest\")