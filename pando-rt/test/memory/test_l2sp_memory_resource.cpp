// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory_resource.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"

namespace {

pando::GlobalPtr<void> allocateInPlace(std::size_t size) {
  auto resource = pando::getDefaultL2SPResource();
  return resource->allocate(size);
}

void deallocateInPlace(std::size_t size, pando::GlobalPtr<void> pointer) {
  auto resource = pando::getDefaultL2SPResource();
  resource->deallocate(pointer, size);
}

} // namespace

TEST(L2SPMemoryResource, Allocate) {
  const std::size_t size = 8;
  pando::GlobalPtr<void> pointer = allocateInPlace(size);
  EXPECT_NE(pointer, nullptr);
  deallocateInPlace(size, pointer);
}

TEST(L2SPMemoryResource, AllocateLarge) {
  const std::size_t size = 1024;
  pando::GlobalPtr<void> pointer = allocateInPlace(size);
  EXPECT_NE(pointer, nullptr);
  deallocateInPlace(size, pointer);
}
