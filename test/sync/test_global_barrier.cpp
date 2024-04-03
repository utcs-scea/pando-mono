// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/sync/global_barrier.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

constexpr pando::Place anyZero = pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};

TEST(GlobalBarrier, Initialize) {
  galois::GlobalBarrier gb;
  EXPECT_EQ(gb.initialize(1), pando::Status::Success);
  gb.deinitialize();
}

TEST(GlobalBarrier, Wait) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::GlobalBarrier gb;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 1);
  *ptr = 0;
  EXPECT_EQ(gb.initialize(1), pando::Status::Success);
  auto func =
      +[](galois::GlobalBarrier gb, pando::GlobalPtr<std::uint64_t> ptr, std::uint64_t goodVal) {
        *ptr = goodVal;
        gb.done();
      };
  EXPECT_EQ(pando::executeOn(anyZero, func, gb, ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(gb.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  gb.deinitialize();
}

TEST(GlobalBarrier, AddOne) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::GlobalBarrier gb;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(gb.initialize(0), pando::Status::Success);
  *ptr = 0;
  gb.addOne();
  auto func =
      +[](galois::GlobalBarrier gb, pando::GlobalPtr<std::uint64_t> ptr, std::uint64_t goodVal) {
        *ptr = goodVal;
        gb.done();
      };
  EXPECT_EQ(pando::executeOn(anyZero, func, gb, ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(gb.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  gb.deinitialize();
}

TEST(GlobalBarrier, Add) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::GlobalBarrier gb;
  pando::GlobalPtr<std::uint64_t> ptr;
  pando::LocalStorageGuard ptrGuard(ptr, 10);
  EXPECT_EQ(gb.initialize(0), pando::Status::Success);
  *ptr = 0;
  gb.add(10);
  auto func =
      +[](galois::GlobalBarrier gb, pando::GlobalPtr<std::uint64_t> ptr, std::uint64_t goodVal) {
        for (std::uint64_t i = 0; i < 9; i++) {
          gb.done();
        }
        *ptr = goodVal;
        gb.done();
      };
  EXPECT_EQ(pando::executeOn(anyZero, func, gb, ptr, goodVal), pando::Status::Success);
  EXPECT_EQ(gb.wait(), pando::Status::Success);
  EXPECT_EQ(*ptr, goodVal);
  gb.deinitialize();
}

TEST(GlobalBarrier, SingleWait) {
  auto dims = pando::getPlaceDims();
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::GlobalBarrier gb;
  EXPECT_EQ(gb.initialize(dims.node.id), pando::Status::Success);
  pando::Array<uint64_t> array;
  EXPECT_EQ(array.initialize(dims.node.id), pando::Status::Success);
  array.fill(0ul);
  auto func =
      +[](galois::GlobalBarrier gb, pando::Array<std::uint64_t> array, std::uint64_t goodVal) {
        array[pando::getCurrentPlace().node.id] = goodVal;
        gb.done();
      };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, gb, array, goodVal),
        pando::Status::Success);
  }
  EXPECT_EQ(gb.wait(), pando::Status::Success);
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(array[nodeId], goodVal);
  }
  gb.deinitialize();
}

TEST(GlobalBarrier, MultipleWait) {
  auto dims = pando::getPlaceDims();
  constexpr std::uint64_t goodVal = 0xDEADBEEF;
  galois::GlobalBarrier gb1, gb2;
  EXPECT_EQ(gb1.initialize(1), pando::Status::Success);
  EXPECT_EQ(gb2.initialize(dims.node.id), pando::Status::Success);
  pando::Array<uint64_t> array;
  EXPECT_EQ(array.initialize(dims.node.id), pando::Status::Success);
  array.fill(0ul);

  // host 0
  array[pando::getCurrentPlace().node.id] = goodVal;

  auto func = +[](galois::GlobalBarrier gb1, galois::GlobalBarrier gb2,
                  pando::Array<std::uint64_t> array, std::uint64_t goodVal) {
    EXPECT_EQ(gb1.wait(), pando::Status::Success);
    array[pando::getCurrentPlace().node.id] = goodVal;
    gb2.done();
  };
  for (std::int16_t nodeId = 1; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, gb1, gb2, array, goodVal),
        pando::Status::Success);
  }

  EXPECT_EQ(array[0], goodVal);
  for (std::int16_t nodeId = 1; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(array[nodeId], 0ul);
  }

  gb1.done();
  gb2.done();
  EXPECT_EQ(gb2.wait(), pando::Status::Success);

  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(array[nodeId], goodVal);
  }

  gb1.deinitialize();
  gb2.deinitialize();
}
