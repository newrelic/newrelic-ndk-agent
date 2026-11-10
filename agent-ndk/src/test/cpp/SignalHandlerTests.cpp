/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <csignal>
#include <cstdlib>
#include <thread>
#include <vector>
#include <unistd.h>

#include "signal-handler.h"

namespace {

    stack_t current_sigstack() {
        stack_t stack = {};
        EXPECT_EQ(0, sigaltstack(nullptr, &stack));
        return stack;
    }

    void disable_sigstack() {
        stack_t stack = {};
        stack.ss_flags = SS_DISABLE;
        EXPECT_EQ(0, sigaltstack(&stack, nullptr));
    }

    struct sigaction current_action(int signo) {
        struct sigaction action = {};
        EXPECT_EQ(0, sigaction(signo, nullptr, &action));
        return action;
    }

    // Handlers are installed by a helper thread: wait for them to differ from the originals
    void wait_for_handlers(int signo, const struct sigaction &original) {
        for (int i = 0; i < 200; i++) {
            if (current_action(signo).sa_sigaction != original.sa_sigaction) {
                return;
            }
            usleep(10000);
        }
    }

}   // namespace

/**
 * sigaltstack is per-thread, and the runtime sets up a stack on every thread it creates.
 * Replacing it with one the agent later frees leaves the thread delivering signals on freed memory.
 */
TEST(SignalHandlerTest, InitializeKeepsTheAlternateStackOfTheCallingThread) {
    std::thread([] {
        std::vector<char> ownStack(SIGSTKSZ * 4);
        stack_t own = {};
        own.ss_sp = ownStack.data();
        own.ss_size = ownStack.size();
        ASSERT_EQ(0, sigaltstack(&own, nullptr));

        const struct sigaction original = current_action(SIGSEGV);
        signal_handler_initialize();
        wait_for_handlers(SIGSEGV, original);
        signal_handler_shutdown();

        const stack_t after = current_sigstack();
        EXPECT_EQ(own.ss_sp, after.ss_sp);
        EXPECT_EQ(own.ss_size, after.ss_size);

        disable_sigstack();     // before ownStack goes out of scope
    }).join();
}

/**
 * A stack the agent installs on a thread has to outlive the agent being stopped: nothing
 * unregisters it from the thread, so freeing it leaves the thread with a dangling stack.
 */
TEST(SignalHandlerTest, ShutdownDoesNotFreeTheAlternateStackItInstalled) {
    std::thread([] {
        disable_sigstack();

        const struct sigaction original = current_action(SIGSEGV);
        signal_handler_initialize();
        wait_for_handlers(SIGSEGV, original);

        const stack_t installed = current_sigstack();
        ASSERT_EQ(0, installed.ss_flags & SS_DISABLE) << "no alternate stack was installed";

        signal_handler_shutdown();

        // A freed block of this size is the first thing the allocator hands back
        std::vector<void *> blocks;
        for (int i = 0; i < 64; i++) {
            blocks.push_back(malloc(installed.ss_size));
        }
        for (void *block: blocks) {
            EXPECT_NE(installed.ss_sp, block) << "the installed alternate stack was freed";
            free(block);
        }

        disable_sigstack();
    }).join();
}
