// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#include "pando-rt/memory/allocate_memory.hpp"

#include "pando-rt/stdlib.hpp"

namespace pando {

namespace detail {

[[nodiscard]] GlobalPtr<void> allocateMemoryImpl(std::uint64_t size, MemoryType memoryType) {
  switch (memoryType) {
    case MemoryType::Main:
      return getDefaultMainMemoryResource()->allocate(size);
    case MemoryType::L2SP:
      return getDefaultL2SPResource()->allocate(size);
    default:
      return nullptr;
  }
}

void deallocateMemoryImpl(GlobalPtr<void> p, std::uint64_t size) {
  switch (memoryTypeOf(p)) {
    case MemoryType::Main:
      getDefaultMainMemoryResource()->deallocate(p, size);
      break;
    case MemoryType::L2SP:
      getDefaultL2SPResource()->deallocate(p, size);
      break;
    default:
      PANDO_ABORT("Cannot deallocate memory");
      break;
  }
}

} // namespace detail

} // namespace pando
