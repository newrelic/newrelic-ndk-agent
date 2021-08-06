##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

cmake_minimum_required(VERSION 3.11.0)

# Declares and names the project.

project("agent-ndk")

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

include_directories("./include")

add_library(agent-ndk SHARED
        agent-ndk.cpp
        signal-handler.cpp
        terminate-handler.cpp
        unwinder.cpp
        anr-handler.cpp
        procfs.cpp
        signal-utils.cpp
        )


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

# NDK support libs
find_library(log-lib log)

target_link_libraries(agent-ndk ${log-lib})

##
# Include testing framework(s) and test app
##

include(UnitTests.cmake)

