// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <tuple>

#include "pando-rt/execution/bulk_execute_on.hpp"

#include "pando-rt/specific_storage.hpp"
#include "pando-rt/sync/atomic.hpp"
#include "pando-rt/sync/wait.hpp"

#include "../common.hpp"

namespace {

pando::NodeSpecificStorage<std::int64_t> counter;

struct IncrementCounter {
  void operator()(std::int64_t val) {
    pando::atomicIncrement(counter.getPointer(), val, std::memory_order_relaxed);
  }
};

struct IncrementCounters {
  void operator()(std::int64_t x, std::int64_t y) {
    pando::atomicIncrement(&counter, x, std::memory_order_relaxed);
    pando::atomicIncrement(&counter, y, std::memory_order_relaxed);
  }
};

} // namespace

TEST(BulkExecuteOn, ThisNode) {
  counter = 0;

  const auto& thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  EXPECT_EQ(pando::bulkExecuteOn(place, IncrementCounter{}, std::make_tuple(1), std::make_tuple(2),
                                 std::make_tuple(3), std::make_tuple(4)),
            pando::Status::Success);

  pando::waitUntil([&] {
    return pando::atomicLoad(&counter, std::memory_order_relaxed) == 10;
  });

  EXPECT_EQ(pando::atomicLoad(&counter, std::memory_order_relaxed), 10);
}

TEST(BulkExecuteOn, ThisNodeAnyPod) {
  counter = 0;

  EXPECT_EQ(pando::bulkExecuteOn(pando::anyPod, IncrementCounter{}, std::make_tuple(1),
                                 std::make_tuple(2), std::make_tuple(3), std::make_tuple(4)),
            pando::Status::Success);

  pando::waitUntil([&] {
    return pando::atomicLoad(&counter, std::memory_order_relaxed) == 10;
  });

  EXPECT_EQ(pando::atomicLoad(&counter, std::memory_order_relaxed), 10);
}

TEST(BulkExecuteOn, ThisNodeAnyCore) {
  counter = 0;

  EXPECT_EQ(pando::bulkExecuteOn(pando::anyCore, IncrementCounter{}, std::make_tuple(1),
                                 std::make_tuple(2), std::make_tuple(3), std::make_tuple(4)),
            pando::Status::Success);

  pando::waitUntil([&] {
    return pando::atomicLoad(&counter, std::memory_order_relaxed) == 10;
  });

  EXPECT_EQ(pando::atomicLoad(&counter, std::memory_order_relaxed), 10);
}

TEST(BulkExecuteOn, ThisNodeMultipleArgs) {
  counter = 0;

  const auto& thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  EXPECT_EQ(
      pando::bulkExecuteOn(place, IncrementCounters{}, std::make_tuple(1, 1), std::make_tuple(2, 2),
                           std::make_tuple(3, 3), std::make_tuple(4, 4)),
      pando::Status::Success);

  pando::waitUntil([&] {
    return pando::atomicLoad(&counter, std::memory_order_relaxed) == 20;
  });

  EXPECT_EQ(pando::atomicLoad(&counter, std::memory_order_relaxed), 20);
}
