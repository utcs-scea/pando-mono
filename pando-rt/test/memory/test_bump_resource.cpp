// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "gtest/gtest.h"

#include "../common_memory.hpp"
#include "pando-rt/memory/bump_memory_resource.hpp"

namespace {

template <std::uint64_t Capacity>
struct TestData {
  static constexpr auto capacity = Capacity;
};

template <class TestData>
struct BumpMemoryResourceTest : public ::testing::Test {};
using BumpMemoryResourceTestTypes = ::testing::Types<TestData<64>, TestData<65>, TestData<128>,
                                                     TestData<250>, TestData<700>, TestData<200>>;

TYPED_TEST_SUITE(BumpMemoryResourceTest, BumpMemoryResourceTestTypes);

} // namespace

TYPED_TEST(BumpMemoryResourceTest, AllocateTest) {
  static constexpr std::uint32_t minimumAlignment = 1;
  using BumpResourceType = pando::BumpMemoryResource<minimumAlignment>;
  static constexpr auto capacity = TypeParam::capacity;
  static constexpr auto overhead = BumpResourceType::computeMetadataSize();
  static constexpr auto totalBytes = capacity + overhead;
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  BumpResourceType memoryResource(buffer, totalBytes);
  const std::size_t size = 2;
  auto ptr = memoryResource.allocate(size);
  EXPECT_NE(ptr, nullptr);
  memoryResource.deallocate(ptr, size);
}

TYPED_TEST(BumpMemoryResourceTest, ExhaustTest) {
  static constexpr std::uint32_t minimumAlignment = 1;
  using BumpResourceType = pando::BumpMemoryResource<minimumAlignment>;
  static constexpr auto capacity = TypeParam::capacity;
  static constexpr auto overhead = BumpResourceType::computeMetadataSize();
  static constexpr auto totalBytes = capacity + overhead;
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::GlobalPtr<std::byte> bufferEnd = buffer + totalBytes;
  BumpResourceType memoryResource(buffer, totalBytes);
  auto successfulAllocation = memoryResource.allocate(capacity);
  EXPECT_NE(successfulAllocation, nullptr);
  EXPECT_LE(buffer, successfulAllocation);
  EXPECT_GT(successfulAllocation, buffer);
  memoryResource.deallocate(successfulAllocation, capacity);

  auto FailedAllocation = memoryResource.allocate(1);
  EXPECT_EQ(FailedAllocation, nullptr);
}
