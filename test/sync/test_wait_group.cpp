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
  galois::WaitGroup wg;
  EXPECT_EQ(wg.initialize(1), pando::Status::Success);
  wg.deinitialize();
}

TEST(WaitGroup, Wait) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 1);
  *ptr = 0;
  EXPECT_EQ(wg.initialize(1), pando::Status::Success);
  auto func = +[](galois::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    *ptr = goodVal;
    wg.done();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}

TEST(WaitGroup, AddOne) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  *ptr = 0;
  wg.getHandle().addOne();
  auto func = +[](galois::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    *ptr = goodVal;
    wg.done();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}

TEST(WaitGroup, Add) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  *ptr = 0;
  wg.getHandle().add(10);
  auto func = +[](galois::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    for (std::uint64_t i = 0; i < 9; i++) {
      wg.done();
    }
    *ptr = goodVal;
    wg.done();
  };
  EXPECT_EQ(pando::executeOn(anyZero, func, wg.getHandle(), ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  wg.deinitialize();
}

TEST(WaitGroup, RemoteUsage) {
  auto dims = pando::getPlaceDims();
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::WaitGroup wg;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, dims.node.id);
  EXPECT_EQ(wg.initialize(dims.node.id), pando::Status::Success);
  auto func = +[](galois::WaitGroup::HandleType wg, pando::GlobalPtr<std::uint64_t> ptr,
                  std::uint64_t goodVal) {
    ptr[pando::getCurrentPlace().node.id] = goodVal;
    wg.done();
  };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, wg.getHandle(), ptr, goodVal),
        pando::Status::Success);
  }
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(*ptr, goodVal);
  }
  wg.deinitialize();
}
