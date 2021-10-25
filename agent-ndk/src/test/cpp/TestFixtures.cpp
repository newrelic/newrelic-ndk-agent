/**
 *
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <gtest/gtest.h>

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestWithParam;
using ::testing::TestPartResult;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;
using ::testing::UnitTest;

namespace ndk {

// A test fixture is a place to hold objects and functions shared by
// all tests in a test case.  Using a test fixture avoids duplicating
// the test code necessary to initialize and cleanup those common
// objects for each test.  It is also useful for defining sub-routines
// that your tests need to invoke a lot.

// In this sample, we want to ensure that every test finishes within
// ~5 seconds.  If a test takes longer to run, we consider it a
// failure.
//
// We put the code for timing a test in a test fixture called
// "QuickTest".  QuickTest is intended to be the super fixture that
// other fixtures derive from, therefore there is no test case with
// the name "QuickTest".  This is OK.
//
// Later, we will derive multiple test fixtures from QuickTest.
class QuickTest : public testing::Test {
    const int TIME_MAX = 5;
protected:
    // Remember that SetUp() is run immediately before a test starts.
    // This is a good place to record the start time.
    void SetUp() override { start_time_ = time(nullptr); }

    // TearDown() is invoked immediately after a test finishes.  Here we
    // check if the test was too slow.
    void TearDown() override {
        // Gets the time when the test finishes
        const time_t end_time = time(nullptr);

        // Asserts that the test took no more than ~5 seconds.  Did you
        // know that you can use assertions in SetUp() and TearDown() as
        // well?
        EXPECT_TRUE(end_time - start_time_ <= TIME_MAX) << "The test took too long.";
    }

    // The UTC time (in seconds) when the test starts
    time_t start_time_;
};


// We derive a fixture named IntegerFunctionTest from the QuickTest
// fixture.  All tests using this fixture will be automatically
// required to be quick.
class IntegerFunctionTest : public QuickTest {
  // We don't need any more logic than already in the QuickTest fixture.
  // Therefore the body is empty.
};

// Derive a a test fixture class from testing::Test
class TestFixtures : public testing::Test {

    protected:
    // virtual void SetUp() will be called before each test is run.  You
    // should define it if you need to initialize the variables.
    // Otherwise, this can be skipped.
    void SetUp() override {
        // TODO
    }

    // virtual void TearDown() will be called after each test is run.
    // You should define it if there is cleanup work to do.  Otherwise,
    // you don't have to provide it.
    //
    virtual void TearDown() override {
        // TODO
    }

    // A helper function that some test uses.
    static void* helper() {
        // TODO
        return nullptr;
    }

    // Declare variables the tests use
    // TODO
};

// Use TEST_F instead of TEST to define a fixture

// Tests the default c'tor.
TEST_F(TestFixtures, DefaultConstructor) {
}

// Tests installation
TEST_F(TestFixtures, InstallHandler) {
}


}  // namespace