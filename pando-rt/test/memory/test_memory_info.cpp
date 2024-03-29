// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory/memory_info.hpp"

TEST(MemoryInfo, Stack) {
  EXPECT_GE(pando::getThreadStackSize(), 1024);
  EXPECT_EQ(pando::getThreadAvailableStack(), 0);
}

TEST(MemoryInfo, L2SP) {
  EXPECT_GE(pando::getNodeL2SPSize(), 1024);
}

TEST(MemoryInfo, Main) {
  EXPECT_GE(pando::getNodeMainMemorySize(), 1024);
}
