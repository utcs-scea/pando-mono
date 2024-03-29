// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "specific_storage.hpp"

#include "pando-rt/locality.hpp"
#include "pando-rt/stddef.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/memory.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/drvx.hpp"
#endif

namespace pando {

namespace {

// Returns a size aligned to the given alignment
template <typename UIntType>
constexpr UIntType alignSize(UIntType size, UIntType alignment) noexcept {
  return (size + (alignment - 1)) & ~(alignment - 1);
}

#if defined(PANDO_RT_USE_BACKEND_DRVX)

// Converts ROOT memory type to DrvX
constexpr DrvAPI::DrvAPIMemoryType convertMemoryTypeToDrvXSection(MemoryType memoryType) noexcept {
  switch (memoryType) {
    case MemoryType::L1SP:
      return DrvAPI::DrvAPIMemoryL1SP;
    case MemoryType::L2SP:
      return DrvAPI::DrvAPIMemoryL2SP;
    case MemoryType::Main:
      return DrvAPI::DrvAPIMemoryDRAM;
    default:
      return DrvAPI::DrvAPIMemoryNTypes;
  }
}

#endif // PANDO_RT_USE_BACKEND_DRVX

#if defined(PANDO_RT_USE_BACKEND_PREP)

// Reserved space in main memory for global variables
std::size_t mainMemoryReservedSpace = 0;

// Reserved space in L2SP for global variables
std::size_t l2SPMemoryReservedSpace = 0;

#endif // PANDO_RT_USE_BACKEND_PREP

} // namespace

namespace detail {

std::size_t reserveZeroInitL2SPMemory(std::size_t size, std::size_t alignment) {
  if (alignment > alignof(MaxAlignT)) {
    // memories are malloced which means that they are aligned at alignof(std::max_align_t)
    // if we need larger alignment requirements, we need to use aligned_alloc
    PANDO_ABORT("Unsupported alignment");
  }

#if defined(PANDO_RT_USE_BACKEND_PREP)

  l2SPMemoryReservedSpace = alignSize(l2SPMemoryReservedSpace, alignment); // align memory
  const auto allocatedSpace = l2SPMemoryReservedSpace;      // mark address to store object
  const auto alignedSize = alignSize<std::size_t>(size, 8); // align object size to 8 bytes
  l2SPMemoryReservedSpace += alignedSize;                   // reserve space for the object
  return allocatedSpace;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  auto& section = DrvAPI::DrvAPISection::GetSection(DrvAPI::DrvAPIMemoryL2SP);
  const auto currentSize = section.getSize();
  const auto alignDelta = alignSize<std::uint64_t>(currentSize, alignment) - currentSize;
  section.increaseSizeBy(alignDelta);  // align memory
  return section.increaseSizeBy(size); // reserve space for the object

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

std::size_t reserveZeroInitMainMemory(std::size_t size, std::size_t alignment) {
  if (alignment > alignof(MaxAlignT)) {
    // memories are malloced which means that they are aligned at alignof(std::max_align_t)
    // if we need larger alignment requirements, we need to use aligned_alloc
    PANDO_ABORT("Unsupported alignment");
  }

#if defined(PANDO_RT_USE_BACKEND_PREP)

  mainMemoryReservedSpace = alignSize(mainMemoryReservedSpace, alignment); // align memory
  const auto allocatedSpace = mainMemoryReservedSpace;      // mark address to store object
  const auto alignedSize = alignSize<std::size_t>(size, 8); // align object size to 8 bytes
  mainMemoryReservedSpace += alignedSize;                   // reserve space for the object
  return allocatedSpace;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  auto& section = DrvAPI::DrvAPISection::GetSection(DrvAPI::DrvAPIMemoryDRAM);
  const auto currentSize = section.getSize();
  const auto alignDelta = alignSize<std::uint64_t>(currentSize, alignment) - currentSize;
  section.increaseSizeBy(alignDelta);  // align memory
  return section.increaseSizeBy(size); // reserve space for the object

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace detail

std::size_t getReservedMemorySpace(MemoryType memoryType) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  switch (memoryType) {
    case MemoryType::L2SP:
      return l2SPMemoryReservedSpace;
    case MemoryType::Main:
      return mainMemoryReservedSpace;
    case MemoryType::L1SP:
    default:
      PANDO_ABORT("Unsupported memory type");
      return 0;
  }

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  const auto sectionType = convertMemoryTypeToDrvXSection(memoryType);
  const auto& section = DrvAPI::DrvAPISection::GetSection(sectionType);
  return section.getSize();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace pando
