// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory_resource.hpp"

namespace {

pando::GlobalPtr<void> allocateInPlace(std::size_t size) {
  auto resource = pando::getDefaultMainMemoryResource();
  return resource->allocate(size);
}

void deallocateInPlace(std::size_t size, pando::GlobalPtr<void> pointer) {
  auto resource = pando::getDefaultMainMemoryResource();
  resource->deallocate(pointer, size);
}

struct Container {
  pando::GlobalPtr<std::uint64_t> data;
  std::size_t size = 0;
};

} // namespace

TEST(MainMemoryResource, Allocate) {
  std::size_t size = 8;
  pando::GlobalPtr<void> pointer = allocateInPlace(size);
  EXPECT_NE(pointer, nullptr);
  deallocateInPlace(size, pointer);
}

TEST(MainMemoryResource, AllocateLarge) {
  std::size_t size = 1024;
  pando::GlobalPtr<void> pointer = allocateInPlace(size);
  EXPECT_NE(pointer, nullptr);
  deallocateInPlace(size, pointer);
}

TEST(MainMemoryResource, MultipleLargeAllocations) {
  std::size_t size = 100000000;

  auto mmr = pando::getDefaultMainMemoryResource();

  auto globalContainer = static_cast<pando::GlobalPtr<Container>>(mmr->allocate(sizeof(Container)));
  ASSERT_NE(globalContainer, nullptr);
  Container c = *globalContainer;
  auto array = mmr->allocate(sizeof(std::uint64_t) * size);
  ASSERT_NE(array, nullptr);

  c.data = static_cast<pando::GlobalPtr<std::uint64_t>>(array);
  c.size = size;
  *globalContainer = c;

  pando::GlobalPtr<std::uint64_t> globalArrayFirst = &c.data[0];
  pando::GlobalPtr<std::uint64_t> globalArrayLast = &c.data[size - 1];

  // The container storage must not overlap with the array storage
  EXPECT_FALSE((static_cast<pando::GlobalPtr<void>>(globalContainer) >=
                static_cast<pando::GlobalPtr<void>>(globalArrayFirst)) &&
               (static_cast<pando::GlobalPtr<void>>(globalContainer) <=
                static_cast<pando::GlobalPtr<void>>(globalArrayLast)));

  mmr->deallocate(array, sizeof(std::uint64_t) * size);
  mmr->deallocate(globalContainer, sizeof(Container));
}
