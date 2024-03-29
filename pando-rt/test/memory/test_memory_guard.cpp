// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory/memory_guard.hpp"

namespace {

void simpleLoopTest(std::uint64_t size, std::uint64_t loopCount) {
  for (std::uint64_t i = 0; i < loopCount; i++) {
    pando::GlobalPtr<std::uint64_t> g;
    pando::LocalStorageGuard guardG(g, size);
    EXPECT_TRUE(g != nullptr);
    g[0] = i;
    g[size - 1] = i;
    std::uint64_t g0 = g[0];
    EXPECT_EQ(g0, i);
    std::uint64_t gSize = g[size - 1];
    EXPECT_EQ(gSize, i);
  }
}

} // namespace

TEST(LocalStorageGuard, CheckSmallAllocation) {
  simpleLoopTest(4, 3);
}

TEST(LocalStorageGuard, CheckLargeAllocation) {
  const std::uint64_t size = (1ull << 10);
  simpleLoopTest(size, 3);
}

TEST(LocalStorageGuard, CheckLimitAllocation) {
  const std::uint64_t size = (1ull << 10);
  simpleLoopTest(size, 32);
}
