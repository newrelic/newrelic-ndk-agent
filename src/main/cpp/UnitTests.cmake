##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
##

##
# Testing frameworks: we will choose one based on usability
#

cmake_minimum_required(VERSION 3.18.0)
enable_testing()

include(CTest)
include(FetchContent)

# GTest/GMock: https://google.github.io/googletest/primer.html
#

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG master
)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)


set(TEST_SOURCES
        ${PROJECT_TEST_DIR}/SimpleTests.cpp
        ${PROJECT_TEST_DIR}/TestFixtures.cpp
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

gtest_discover_tests(agent-ndk-test)


##
# Catch: https://github.com/catchorg/Catch2
#

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.1)

FetchContent_MakeAvailable(Catch2)

# add_executable(ndk-catch-test # catch.cpp )
# target_link_libraries(ndk-catch-test PRIVATE Catch2::Catch2)


if (FIXME)
    include(Catch)
    catch_discover_tests(${TEST_APP})
endif ()

