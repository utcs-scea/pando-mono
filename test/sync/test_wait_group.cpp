// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

constexpr pando::Place anyZero = pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};

TEST(WaitGroup, Initialize) {
  pando::WaitGroup wg;
  EXPECT_EQ(wg.initialize(1), pando::Status::Success);
  wg.deinitialize();
}

TEST(WaitGroup, Wait) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  pando::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 1);
  *ptr = 0;
  EXPECT_EQ(wg.initialize(1), pando::Status::Success);
  auto func = +[](pando::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    *ptr = goodVal;
    wg.arrive();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  wg.wait();
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}

TEST(WaitGroup, AddOne) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  pando::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  *ptr = 0;
  wg.getHandle().addOne();
  auto func = +[](pando::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    *ptr = goodVal;
    wg.arrive();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  wg.wait();
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}

TEST(WaitGroup, Add) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  pando::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  *ptr = 0;
  wg.getHandle().add(10);
  auto func = +[](pando::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    for (std::uint64_t i = 0; i < 9; i++) {
      wg.arrive();
    }
    *ptr = goodVal;
    wg.arrive();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  wg.wait();
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}
