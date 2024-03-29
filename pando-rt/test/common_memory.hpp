// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_TEST_COMMON_MEMORY_HPP_
#define PANDO_RT_TEST_COMMON_MEMORY_HPP_

#include "pando-rt/memory/address_translation.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "pando-rt/memory/memory_info.hpp"
#include "pando-rt/memory_resource.hpp"
#include "pando-rt/stddef.hpp"

inline pando::GlobalPtr<std::byte> getMainMemoryStart() noexcept {
  auto memoryInformation = pando::detail::getMemoryStartAndSize(pando::MemoryType::Main);
  return memoryInformation.first;
}

// Assuming `ptr` is aligned, the function bumps the pointer to the next aligned size that is
// multiple of alignof(pando::MaxAlignT).
inline pando::GlobalPtr<std::byte> alignedBumpPointer(pando::GlobalPtr<std::byte> ptr,
                                                      std::size_t size) {
  auto alignment = alignof(pando::MaxAlignT);
  auto numBlocks = (size + alignment - 1) / alignment;
  return ptr + alignment * numBlocks;
}
#endif // PANDO_RT_TEST_COMMON_MEMORY_HPP_
