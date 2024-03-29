// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory/allocate_memory.hpp"

#include "pando-rt/sync/notification.hpp"

TEST(AllocateMemory, MainFromCP) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  const auto thisPlace = pando::getCurrentPlace();
  {
    // current node
    const pando::Place place{thisPlace.node, pando::anyPod, pando::anyCore};

    auto result = pando::allocateMemory<ValueType>(n, place, pando::MemoryType::Main);
    EXPECT_TRUE(result.hasValue());

    auto ptr = result.value();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pando::memoryTypeOf(ptr), pando::MemoryType::Main);
    EXPECT_EQ(pando::localityOf(ptr), place);

    pando::deallocateMemory(ptr, n);
  }

  {
    // other node
    const pando::NodeIndex nodeIdx((thisPlace.node.id + 1) % pando::getNodeDims().id);
    const pando::Place place{nodeIdx, pando::anyPod, pando::anyCore};

    auto result = pando::allocateMemory<ValueType>(n, place, pando::MemoryType::Main);
    EXPECT_TRUE(result.hasValue());

    auto ptr = result.value();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pando::memoryTypeOf(ptr), pando::MemoryType::Main);
    EXPECT_EQ(pando::localityOf(ptr), place);

    pando::deallocateMemory(ptr, n);
  }
}

TEST(AllocateMemory, L2SPFromCP) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::anyPod, pando::anyCore};

  auto result = pando::allocateMemory<ValueType>(n, place, pando::MemoryType::L2SP);
  EXPECT_TRUE(result.hasValue());

  auto ptr = result.value();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(pando::memoryTypeOf(ptr), pando::MemoryType::L2SP);

  pando::deallocateMemory(ptr, n);
}

TEST(AllocateMemory, L1SPFromCP) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  auto result = pando::allocateMemory<ValueType>(n, pando::anyPlace, pando::MemoryType::L1SP);
  EXPECT_FALSE(result.hasValue());
  EXPECT_EQ(result.error(), pando::Status::InvalidValue);
}

TEST(AllocateMemory, MainFromHart) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  auto allocateF = +[] {
    const auto thisPlace = pando::getCurrentPlace();

    auto result = pando::allocateMemory<ValueType>(n, thisPlace, pando::MemoryType::Main);
    EXPECT_TRUE(result.hasValue());

    return result.value();
    auto ptr = result.value();

    return ptr;
  };

  auto deallocateF = +[](pando::GlobalPtr<ValueType> ptr) {
    pando::deallocateMemory(ptr, n);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::anyPod, pando::anyCore};
  auto allocateResult = pando::executeOnWait(place, allocateF);
  EXPECT_TRUE(allocateResult.hasValue());

  auto ptr = allocateResult.value();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(pando::memoryTypeOf(ptr), pando::MemoryType::Main);
  EXPECT_EQ(pando::localityOf(ptr), place);

  auto deallocateResult = pando::executeOnWait(place, deallocateF, ptr);
  EXPECT_TRUE(deallocateResult.hasValue());
}

TEST(AllocateMemory, L2SPFromHart) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  auto allocateF = +[] {
    const auto thisPlace = pando::getCurrentPlace();

    auto result = pando::allocateMemory<ValueType>(n, thisPlace, pando::MemoryType::L2SP);
    EXPECT_TRUE(result.hasValue());

    return result.value();
    auto ptr = result.value();

    return ptr;
  };

  auto deallocateF = +[](pando::GlobalPtr<ValueType> ptr) {
    pando::deallocateMemory(ptr, n);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex(0, 0), pando::anyCore};
  auto allocateResult = pando::executeOnWait(place, allocateF);
  EXPECT_TRUE(allocateResult.hasValue());

  auto ptr = allocateResult.value();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(pando::memoryTypeOf(ptr), pando::MemoryType::L2SP);

  // Currently, L2SP is treated as a single chunk and not partitioned between PODs. There is no
  // guarantee that the allocation result locality is within the same POD.
  EXPECT_EQ(pando::localityOf(ptr).node, place.node);

  auto deallocateResult = pando::executeOnWait(place, deallocateF, ptr);
  EXPECT_TRUE(deallocateResult.hasValue());
}

TEST(AllocateMemory, L1SPFromHart) {
  using ValueType = std::int64_t;
  const auto n = 100u;

  auto allocateF = +[] {
    const auto thisPlace = pando::getCurrentPlace();

    auto result = pando::allocateMemory<ValueType>(n, thisPlace, pando::MemoryType::L1SP);
    EXPECT_FALSE(result.hasValue());
    EXPECT_EQ(result.error(), pando::Status::InvalidValue);

    return nullptr;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex(0, 0), pando::CoreIndex(0, 0)};
  auto allocateResult = pando::executeOnWait(place, allocateF);
  EXPECT_TRUE(allocateResult.hasValue());
  EXPECT_EQ(allocateResult.value(), nullptr);
}

TEST(AllocateMemory, StressMain) {
  const std::uint64_t requests = 10;
  pando::NotificationArray notifications;
  EXPECT_EQ(notifications.init(requests), pando::Status::Success);
  for (std::uint64_t i = 0; i < requests; i++) {
    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore},
                  +[](pando::Notification::HandleType done) {
                    constexpr auto n = 1;
                    const auto thisPlace = pando::getCurrentPlace();
                    auto result =
                        pando::allocateMemory<std::uint64_t>(n, thisPlace, pando::MemoryType::Main);
                    EXPECT_TRUE(result.hasValue());
                    pando::deallocateMemory(result.value(), n);
                    done.notify();
                  },
                  notifications.getHandle(i)),
              pando::Status::Success);
  }
  notifications.wait();
}
