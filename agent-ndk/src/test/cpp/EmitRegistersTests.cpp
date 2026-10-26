/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "emitter.h"

#if defined(__arm__)

// The expected value of the register is its own value, not its index
#define EXPECT_REGISTER(state, name, value) \
    EXPECT_NE(std::string::npos, (state).find("'" name "':'" value "'")) << (state)

TEST(EmitRegistersTest, ArmRegistersReportTheirValues) {
    ucontext_t context;
    std::memset(&context, 0, sizeof(context));

    mcontext_t &mcontext = context.uc_mcontext;
    mcontext.arm_r0 = 0xa0000000;
    mcontext.arm_r1 = 0xa0000001;
    mcontext.arm_r2 = 0xa0000002;
    mcontext.arm_r3 = 0xa0000003;
    mcontext.arm_r4 = 0xa0000004;
    mcontext.arm_r5 = 0xa0000005;
    mcontext.arm_r6 = 0xa0000006;
    mcontext.arm_r7 = 0xa0000007;
    mcontext.arm_r8 = 0xa0000008;
    mcontext.arm_r9 = 0xa0000009;
    mcontext.arm_r10 = 0xa000000a;
    mcontext.arm_fp = 0xb000000b;
    mcontext.arm_ip = 0xb000000c;
    mcontext.arm_sp = 0xb000000d;
    mcontext.arm_lr = 0xb000000e;
    mcontext.arm_pc = 0xb000000f;
    mcontext.arm_cpsr = 0xb0000010;

    std::string state;
    emit_registers(&context, state);

    EXPECT_REGISTER(state, "r0", "a0000000");
    EXPECT_REGISTER(state, "r1", "a0000001");
    EXPECT_REGISTER(state, "r2", "a0000002");
    EXPECT_REGISTER(state, "r3", "a0000003");
    EXPECT_REGISTER(state, "r4", "a0000004");
    EXPECT_REGISTER(state, "r5", "a0000005");
    EXPECT_REGISTER(state, "r6", "a0000006");
    EXPECT_REGISTER(state, "r7", "a0000007");
    EXPECT_REGISTER(state, "r8", "a0000008");
    EXPECT_REGISTER(state, "r9", "a0000009");
    EXPECT_REGISTER(state, "r10", "a000000a");
    EXPECT_REGISTER(state, "fp", "b000000b");
    EXPECT_REGISTER(state, "ip", "b000000c");
    EXPECT_REGISTER(state, "sp", "b000000d");
    EXPECT_REGISTER(state, "lr", "b000000e");
    EXPECT_REGISTER(state, "pc", "b000000f");
    EXPECT_REGISTER(state, "cpsr", "b0000010");
}

#endif  // defined(__arm__)
