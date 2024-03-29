// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/memory.hpp"

#include "pando-rt/stddef.hpp"

#include "../common.hpp"

TEST(Align, Success) {
  using ValueType = std::byte;

  const auto memoryType = pando::MemoryType::Main;
  const std::size_t size = alignof(pando::MaxAlignT);
  const std::size_t totalBytes = size * 2;
  pando::GlobalPtr<void> ptr = malloc(memoryType, totalBytes);
  ASSERT_NE(ptr, nullptr);

  const auto alignment = alignof(pando::MaxAlignT);

  const std::size_t unalignedOffset = 1;
  const std::size_t alignmentOverhead = alignment - unalignedOffset;
  std::size_t space = totalBytes - unalignedOffset;
  const std::size_t expectedRemainingSpace = space - alignmentOverhead;

  pando::GlobalPtr<void> pointerToAlign = static_cast<pando::GlobalPtr<void>>(
      static_cast<pando::GlobalPtr<ValueType>>(ptr) + unalignedOffset);
  pando::GlobalPtr<void> expectedAlignedPtr = static_cast<pando::GlobalPtr<void>>(
      static_cast<pando::GlobalPtr<ValueType>>(ptr) + alignment);

  auto result = pando::align(alignment, size, pointerToAlign, space);

  EXPECT_EQ(result, expectedAlignedPtr);
  EXPECT_EQ(pointerToAlign, expectedAlignedPtr);
  EXPECT_EQ(space, expectedRemainingSpace);

  free(ptr, totalBytes);
}

TEST(Align, Fail) {
  using ValueType = std::byte;

  const auto memoryType = pando::MemoryType::Main;
  const std::size_t size = alignof(pando::MaxAlignT);
  const std::size_t totalBytes = size * 2;
  pando::GlobalPtr<void> ptr = malloc(memoryType, totalBytes);
  ASSERT_NE(ptr, nullptr);

  const auto alignment = alignof(pando::MaxAlignT);

  const std::size_t unalignedOffset = alignment + 1;

  // alignment will fail and pointer will not change
  {
    std::size_t space = totalBytes - unalignedOffset;
    pando::GlobalPtr<void> pointerToAlign = static_cast<pando::GlobalPtr<void>>(
        static_cast<pando::GlobalPtr<ValueType>>(ptr) + unalignedOffset);
    pando::GlobalPtr<void> pointerToAlignCopy = pointerToAlign;
    pando::GlobalPtr<void> expectedResult(nullptr);

    auto result = pando::align(alignment, size, pointerToAlign, space);

    EXPECT_EQ(pointerToAlign, pointerToAlignCopy);
  }

  // alignment will fail and result is nullptr
  {
    std::size_t space = totalBytes - unalignedOffset;
    pando::GlobalPtr<void> pointerToAlign = static_cast<pando::GlobalPtr<void>>(
        static_cast<pando::GlobalPtr<ValueType>>(ptr) + unalignedOffset);

    pando::GlobalPtr<void> pointerToAlignCopy = pointerToAlign;
    pando::GlobalPtr<void> expectedResult(nullptr);

    auto result = pando::align(alignment, size, pointerToAlign, space);

    EXPECT_EQ(result, expectedResult);
  }

  // alignment will fail and space does not change
  {
    std::size_t space = totalBytes - unalignedOffset;
    pando::GlobalPtr<void> pointerToAlign = static_cast<pando::GlobalPtr<void>>(
        static_cast<pando::GlobalPtr<ValueType>>(ptr) + unalignedOffset);

    const std::size_t expectedRemainingSpace = space;
    pando::GlobalPtr<void> pointerToAlignCopy = pointerToAlign;
    pando::GlobalPtr<void> expectedResult(nullptr);

    auto result = pando::align(alignment, size, pointerToAlign, space);

    EXPECT_EQ(space, expectedRemainingSpace);
  }

  free(ptr, totalBytes);
}
