##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

cmake_minimum_required(VERSION 3.18.0)
enable_testing()

include(CTest)
include(FetchContent)

# GTest/GMock: https://google.github.io/googletest/primer.html
#
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

set(TEST_SRC_DIR ${CMAKE_HOME_DIRECTORY}/../../test/cpp)

set(TEST_SOURCES
        ${TEST_SRC_DIR}/AgentNDKTests.cpp
        ${TEST_SRC_DIR}/TestFixtures.cpp
        )

add_executable(
        agent-ndk-test
        ${TEST_SOURCES}
)

target_link_libraries(
        agent-ndk-test
        agent-ndk
        gtest_main
        gmock_main
)

# TODO Run test on ABI
# gtest_discover_tests(agent-ndk-test)
