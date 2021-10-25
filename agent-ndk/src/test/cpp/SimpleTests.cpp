/**
 *
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Demonstrate some basic assertions:

// TEST has two parameters: the test case name and the test name.
// After using the macro, you should define your test logic between a
// pair of braces.  You can use a bunch of macros to indicate the
// success or failure of a test.  EXPECT_TRUE and EXPECT_EQ are
// examples of such macros.  For a complete list, see gtest.h.

TEST(SimpleTests, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("foo", "bar");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}