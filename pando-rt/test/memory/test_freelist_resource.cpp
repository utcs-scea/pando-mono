// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <vector>

#include "gtest/gtest.h"

#include "../common_memory.hpp"
#include "pando-rt/memory/freelist_memory_resource.hpp"

TEST(FreeListMemoryResource, AllocateEmpty) {
  std::size_t metaDataSize = pando::FreeListMemoryResource::computeMetadataSize();
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::FreeListMemoryResource resource(buffer, metaDataSize);
  std::size_t size = 8;
  auto pointer = resource.allocate(size);
  EXPECT_EQ(pointer, nullptr);
}

TEST(FreeListMemoryResource, Allocate) {
  std::size_t metaDataSize = pando::FreeListMemoryResource::computeMetadataSize();
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::FreeListMemoryResource resource(buffer, metaDataSize);
  buffer = alignedBumpPointer(buffer, metaDataSize);

  std::size_t size = 24;
  resource.deallocate(buffer, size);

  auto freeListAllocation = resource.allocate(size);
  EXPECT_EQ(freeListAllocation, buffer);
}

TEST(FreeListMemoryResource, FindBestFit) {
  std::size_t metaDataSize = pando::FreeListMemoryResource::computeMetadataSize();
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::FreeListMemoryResource resource(buffer, metaDataSize);
  buffer = alignedBumpPointer(buffer, metaDataSize);

  std::vector<std::size_t> sizes{24, 30, 40, 50};
  std::vector<pando::GlobalPtr<std::byte>> allocations;
  for (const auto& s : sizes) {
    buffer = alignedBumpPointer(buffer, s);
    allocations.push_back(buffer);
    resource.deallocate(buffer, s);
  }

  auto targetIdx = 2;
  auto p = resource.allocate(sizes[targetIdx]);
  EXPECT_EQ(p, allocations[targetIdx]);
}

TEST(FreeListMemoryResource, AllocateMultiple) {
  std::size_t metaDataSize = pando::FreeListMemoryResource::computeMetadataSize();
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::FreeListMemoryResource resource(buffer, metaDataSize);
  buffer = alignedBumpPointer(buffer, metaDataSize);

  std::vector<std::size_t> permutation{1, 2, 0, 3};
  std::vector<std::size_t> sizes{24, 30, 40, 50};
  std::vector<pando::GlobalPtr<std::byte>> allocations;
  for (const auto& s : sizes) {
    buffer = alignedBumpPointer(buffer, s);
    allocations.push_back(buffer);
    resource.deallocate(buffer, s);
  }

  for (std::size_t i = 0; i < sizes.size(); i++) {
    auto index = permutation[i];
    auto p = resource.allocate(sizes[index]);
    EXPECT_EQ(p, allocations[index]);
  }
}

TEST(FreeListMemoryResource, ExhaustAndFail) {
  std::size_t metaDataSize = pando::FreeListMemoryResource::computeMetadataSize();
  pando::GlobalPtr<std::byte> buffer = getMainMemoryStart();
  pando::FreeListMemoryResource resource(buffer, metaDataSize);
  buffer = alignedBumpPointer(buffer, metaDataSize);

  std::vector<std::size_t> permutation{1, 2, 0, 3};
  std::vector<std::size_t> sizes{24, 30, 40, 50};
  std::vector<pando::GlobalPtr<std::byte>> allocations;
  for (const auto& s : sizes) {
    buffer = alignedBumpPointer(buffer, s);
    allocations.push_back(buffer);
    resource.deallocate(buffer, s);
  }

  for (std::size_t i = 0; i < sizes.size(); i++) {
    auto index = permutation[i];
    auto p = resource.allocate(sizes[index]);
    EXPECT_EQ(p, allocations[index]);
  }

  auto p = resource.allocate(8);
  EXPECT_EQ(p, nullptr);
}
