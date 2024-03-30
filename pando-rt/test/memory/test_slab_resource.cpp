// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "../common_memory.hpp"
#include "pando-rt/memory/slab_memory_resource.hpp"

namespace {

template <std::uint64_t SlabSize, std::uint64_t Capacity>
struct TestData {
  static constexpr auto slabSize = SlabSize;
  static constexpr auto capacity = Capacity;
};

template <class TestData>
struct SlabMemoryResourceTest : public ::testing::Test {};
using SlabMemoryResourceTestTypes =
    ::testing::Types<TestData<8, 64>, TestData<16, 64>, TestData<32, 128>, TestData<64, 192>,
                     TestData<8, 250>, TestData<16, 700>, TestData<32, 97>, TestData<64, 200>>;

TYPED_TEST_SUITE(SlabMemoryResourceTest, SlabMemoryResourceTestTypes);

} // namespace

TYPED_TEST(SlabMemoryResourceTest, AllocateTest) {
  static constexpr auto slabSize = TypeParam::slabSize;
  static constexpr auto capacity = TypeParam::capacity;
  pando::GlobalPtr<void> mainMemoryStart = getMainMemoryStart();
  auto space = slabSize + capacity;
  pando::GlobalPtr<void> buffer = pando::align(slabSize, capacity, mainMemoryStart, space);
  ASSERT_NE(buffer, nullptr);

  pando::SlabMemoryResource<slabSize> memoryResource(
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(buffer), capacity);
  const std::size_t size = 2;
  auto ptr = memoryResource.allocate(size);
  EXPECT_NE(ptr, nullptr);
  memoryResource.deallocate(ptr, size);
}

TYPED_TEST(SlabMemoryResourceTest, ExhaustTest) {
  static constexpr auto slabSize = TypeParam::slabSize;
  static constexpr auto capacity = TypeParam::capacity;
  pando::GlobalPtr<void> mainMemoryStart = getMainMemoryStart();
  auto space = slabSize + capacity;
  pando::GlobalPtr<void> buffer = pando::align(slabSize, capacity, mainMemoryStart, space);
  ASSERT_NE(buffer, nullptr);

  pando::SlabMemoryResource<slabSize> memoryResource(
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(buffer), capacity);
  pando::GlobalPtr<void> bufferStart = buffer;
  pando::GlobalPtr<void> bufferEnd =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(bufferStart) + capacity;
  const auto maxAllocations = memoryResource.bytesCapacity() / slabSize;
  const std::size_t size = 2;
  std::vector<pando::GlobalPtr<void>> allocations(maxAllocations);
  for (std::size_t i = 0; i < maxAllocations; i++) {
    auto ptr = memoryResource.allocate(size);
    EXPECT_NE(ptr, nullptr);
    EXPECT_LE(bufferStart, ptr);
    EXPECT_GT(bufferEnd, ptr);
    allocations[i] = ptr;
  }
  for (std::size_t i = 0; i < maxAllocations; i++) {
    memoryResource.deallocate(allocations[i], size);
  }
}

TYPED_TEST(SlabMemoryResourceTest, ExhaustFailTest) {
  static constexpr auto slabSize = TypeParam::slabSize;
  static constexpr auto capacity = TypeParam::capacity;
  pando::GlobalPtr<void> mainMemoryStart = getMainMemoryStart();
  auto space = slabSize + capacity;
  pando::GlobalPtr<void> buffer = pando::align(slabSize, capacity, mainMemoryStart, space);
  ASSERT_NE(buffer, nullptr);

  pando::SlabMemoryResource<slabSize> memoryResource(
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(buffer), capacity);
  const auto maxAllocations = memoryResource.bytesCapacity() / slabSize;
  const auto numFailures = 32;
  const auto totalAllocations = maxAllocations + numFailures;
  const std::size_t size = 2;
  std::vector<pando::GlobalPtr<void>> allocations(maxAllocations);
  for (std::size_t i = 0; i < totalAllocations; i++) {
    auto ptr = memoryResource.allocate(size);
    if (i < maxAllocations) {
      EXPECT_NE(ptr, nullptr);
      allocations[i] = ptr;
    } else {
      EXPECT_EQ(ptr, nullptr);
    }
  }
  for (std::size_t i = 0; i < maxAllocations; i++) {
    memoryResource.deallocate(allocations[i], size);
  }
}

// TODO(muhaawad): This test requires some form of concurrency that does not depend on pando-rt
// tasks and hence is only enabled for the PREP backend.
#ifdef PANDO_RT_USE_BACKEND_PREP

TYPED_TEST(SlabMemoryResourceTest, ConcurrentExhaustFailTest) {
  static constexpr auto slabSize = TypeParam::slabSize;
  static constexpr auto capacity = TypeParam::capacity;
  pando::GlobalPtr<void> mainMemoryStart = getMainMemoryStart();
  auto space = slabSize + capacity;
  pando::GlobalPtr<void> buffer = pando::align(slabSize, capacity, mainMemoryStart, space);
  ASSERT_NE(buffer, nullptr);

  pando::SlabMemoryResource<slabSize> memoryResource(
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(buffer), capacity);
  const auto maxAllocations = memoryResource.bytesCapacity() / slabSize;
  const auto numFailures = maxAllocations * 8;
  const auto threadCount = maxAllocations + numFailures;
  const std::size_t size = 2;
  std::vector<pando::GlobalPtr<void>> allocations(threadCount, nullptr);

  std::vector<std::thread> threads;
  for (std::size_t t = 0; t < threadCount; t++) {
    std::thread thread([t, &memoryResource, &allocations] {
      const std::size_t size = 2;
      auto ptr = memoryResource.allocate(size);
      allocations[t] = ptr;
    });
    threads.push_back(std::move(thread));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  const auto numSuccessfulAllocations =
      std::count_if(allocations.cbegin(), allocations.cend(), [](const auto ptr) {
        return ptr != nullptr;
      });

  EXPECT_EQ(numSuccessfulAllocations, maxAllocations);

  for (std::size_t t = 0; t < threadCount; t++) {
    memoryResource.deallocate(allocations[t], size);
  }
}
#endif
