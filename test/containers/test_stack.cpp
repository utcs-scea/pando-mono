// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/stack.hpp>
#include <pando-rt/containers/vector.hpp>

namespace {

const uint64_t canary = 9801;

} // namespace

TEST(Stack, Init) {
  uint64_t val = canary;
  galois::Stack<uint64_t> s1;
  EXPECT_EQ(s1.initialize(0), pando::Status::Success);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_EQ(s1.capacity(), 1);
  EXPECT_EQ(s1.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.emplace(val), pando::Status::Success);
  EXPECT_EQ(s1.size(), 1);
  EXPECT_GT(s1.capacity(), 0);
  val = 73;
  EXPECT_EQ(s1.pop(val), pando::Status::Success);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_GT(s1.capacity(), 0);
  EXPECT_EQ(s1.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_GT(s1.capacity(), 0);

  galois::Stack<uint64_t> s2;
  EXPECT_EQ(s2.initialize(10), pando::Status::Success);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);
  EXPECT_EQ(s2.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.emplace(val), pando::Status::Success);
  EXPECT_EQ(s2.size(), 1);
  EXPECT_EQ(s2.capacity(), 10);
  val = 73;
  EXPECT_EQ(s2.pop(val), pando::Status::Success);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);
  EXPECT_EQ(s2.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);

  s1.deinitialize();
  s2.deinitialize();
}

TEST(Stack, Grow) {
  uint64_t check;
  galois::Stack<uint64_t> s;
  EXPECT_EQ(s.initialize(0), pando::Status::Success);
  EXPECT_EQ(s.size(), 0);
  EXPECT_EQ(s.capacity(), 1);

  for (uint64_t i = 0; i < 101; i++) {
    EXPECT_EQ(s.emplace(canary + i), pando::Status::Success);
    if (i % 5 == 0) {
      EXPECT_EQ(s.pop(check), pando::Status::Success);
      EXPECT_EQ(check, canary + i);
      EXPECT_EQ(s.emplace(canary + i), pando::Status::Success);
    }
  }
  EXPECT_EQ(s.size(), 101);
  for (uint64_t i = 101; i > 0; i--) {
    EXPECT_EQ(s.pop(check), pando::Status::Success);
    EXPECT_EQ(check, canary + i - 1);
  }
  EXPECT_EQ(s.size(), 0);
  EXPECT_EQ(s.pop(check), pando::Status::OutOfBounds);
}

TEST(Stack, DeinitWgh) {
  uint64_t val = canary;
  galois::Stack<uint64_t> s1;
  EXPECT_EQ(s1.initialize(0), pando::Status::Success);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_EQ(s1.capacity(), 1);
  EXPECT_EQ(s1.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.emplace(val), pando::Status::Success);
  EXPECT_EQ(s1.size(), 1);
  EXPECT_GT(s1.capacity(), 0);
  val = 73;
  EXPECT_EQ(s1.pop(val), pando::Status::Success);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_GT(s1.capacity(), 0);
  EXPECT_EQ(s1.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_GT(s1.capacity(), 0);

  galois::Stack<uint64_t> s2;
  EXPECT_EQ(s2.initialize(10), pando::Status::Success);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);
  EXPECT_EQ(s2.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.emplace(val), pando::Status::Success);
  EXPECT_EQ(s2.size(), 1);
  EXPECT_EQ(s2.capacity(), 10);
  val = 73;
  EXPECT_EQ(s2.pop(val), pando::Status::Success);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);
  EXPECT_EQ(s2.pop(val), pando::Status::OutOfBounds);
  EXPECT_EQ(val, canary);
  EXPECT_EQ(s2.size(), 0);
  EXPECT_EQ(s2.capacity(), 10);
  pando::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  s1.deinitialize(wg.getHandle());
  s2.deinitialize(wg.getHandle());
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  wg.deinitialize();
}
