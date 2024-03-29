// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/memory.hpp"

#include <cstdint>

#include "pando-rt/memory/global_ptr.hpp"

namespace pando {

GlobalPtr<void> align(std::size_t alignment, std::size_t size, GlobalPtr<void>& ptr,
                      std::size_t& space) {
  if (space < size) {
    return nullptr;
  }
  const auto uIntPtr = globalPtrReinterpretCast<std::uintptr_t>(ptr);
  const auto alignedUIntPtr = (uIntPtr - 1u + alignment) & -alignment;
  const auto difference = alignedUIntPtr - uIntPtr;
  if (difference > (space - size)) {
    return nullptr;
  } else {
    space -= difference;
    ptr = globalPtrReinterpretCast<GlobalPtr<void>>(alignedUIntPtr);
    return ptr;
  }
}

} // namespace pando
