// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "memory_resources.hpp"

#include <algorithm>

#include "pando-rt/locality.hpp"
#include "pando-rt/memory.hpp"
#include "pando-rt/memory/memory_info.hpp"
#include "pando-rt/stdlib.hpp"
#include "specific_storage.hpp"

namespace pando {

namespace {

/**
 * @brief Aligns the bucket to requested alignment and rounds down the size to the nearest
 * alignment.
 *
 * The function will align the bucket start pointer to satisfy the requested alignment,
 * return the remaining bytes in the input buffer and rounds down the size to the nearest
 * alignment.
 *
 *
 * @tparam Alignment Required alignment.
 * @param slabBucket A slab resource bucket to operate on.
 * @param previousBucket The previous bucket that is layed out in memory before the slab bucket.
 * @param bufferStart The start of the entire memory region buffer.
 * @param bufferSize The size of the entire memory region buffer.
 */
template <std::uint32_t Alignment>
void alignStartAndRound(MemoryBucket& slabBucket, const MemoryBucket& previousBucket,
                        GlobalPtr<const std::byte> bufferStart, const std::size_t bufferSize) {
  std::size_t maximumAlignedChunks =
      static_cast<std::size_t>(slabBucket.ratio * bufferSize) / Alignment;
  slabBucket.bytes = maximumAlignedChunks * Alignment;
  slabBucket.start = previousBucket.start + previousBucket.bytes;
  auto remainingBytes =
      bufferSize - (static_cast<GlobalPtr<const std::byte>>(previousBucket.start) - bufferStart);
  auto resourceStart = static_cast<GlobalPtr<void>>(slabBucket.start);

  const auto alignmentResult = align(Alignment, slabBucket.bytes, resourceStart, remainingBytes);
  if (alignmentResult == nullptr) {
    PANDO_ABORT("Failed to align the a memory bucket");
  }
  slabBucket.start = static_cast<GlobalPtr<std::byte>>(alignmentResult);
}

/**
 * @brief Initializes and aligns the FreeList memory resource bucket.
 *
 * @param freeList The free list memory bucket to initialize and align.
 * @param previousBucket  The previous bucket that is layed out in memory before the free list
 * bucket.
 * @param bufferStart The start of the entire memory region buffer.
 * @param bufferSize The size of the entire memory region buffer.
 */
void initializeFreeListResourceBucket(MemoryBucket& freeList, const MemoryBucket& previousBucket,
                                      GlobalPtr<const std::byte> bufferStart,
                                      const std::size_t bufferSize) {
  freeList.bytes = FreeListMemoryResource::computeMetadataSize();
  freeList.ratio = static_cast<double>(freeList.bytes) / static_cast<double>(bufferSize);
  freeList.start = previousBucket.start + previousBucket.bytes;
  const auto freeListMinimumAlignment = alignof(MaxAlignT);
  auto remainingBytes =
      bufferSize - (static_cast<GlobalPtr<const std::byte>>(freeList.start) - bufferStart);
  auto freeListStart = static_cast<GlobalPtr<void>>(freeList.start);
  auto freeListAlignResult =
      align(freeListMinimumAlignment, freeList.bytes, freeListStart, remainingBytes);
  if (freeListAlignResult == nullptr) {
    PANDO_ABORT("Failed to align the FreeList resource");
  }
  freeList.start = static_cast<GlobalPtr<std::byte>>(freeListAlignResult);
}
/**
 * @brief Initializes and aligns the Bump memory resource bucket.
 *
 * @param bump The bump memory bucket to initialize and align.
 * @param previousBucket  The previous bucket that is layed out in memory before the bump
 * bucket.
 * @param bufferStart The start of the entire memory region buffer.
 * @param bufferSize The size of the entire memory region buffer.
 */
void initializeBumpResourceBucket(MemoryBucket& bump, const MemoryBucket& previousBucket,
                                  const GlobalPtr<std::byte> bufferStart,
                                  const std::size_t bufferSize) {
  bump.start = previousBucket.start + previousBucket.bytes;
  bump.bytes = bufferSize - (bump.start - bufferStart);
  auto bumpMinimumSize = BumpMemoryResource<alignof(MaxAlignT)>::computeMetadataSize();
  auto bumpStart = static_cast<GlobalPtr<void>>(bump.start);
  const auto bumpMinimumAlignement = alignof(MaxAlignT);
  auto remainingBytes = bufferSize - (bump.start - bufferStart);

  auto bumpAlignResult = align(bumpMinimumAlignement, bumpMinimumSize, bumpStart, remainingBytes);
  bump.bytes = remainingBytes;
  // no space left for bump resource means we misconfigured the resource buckets split
  if ((bumpAlignResult == nullptr) || (bump.bytes <= bumpMinimumSize)) {
    PANDO_ABORT("Main memory resource buckets breakdown is misconfigured");
  }
  bump.start = static_cast<GlobalPtr<std::byte>>(bumpAlignResult);
  bump.ratio = static_cast<double>(bump.bytes) / static_cast<double>(bufferSize);
}

template <typename T>
[[nodiscard]] GlobalPtr<void> chainedTryAllocate(std::size_t bytes, std::size_t alignment,
                                                 T* allocator) {
  return allocator->allocate(bytes, alignment);
}

template <typename First, typename... Rest>
[[nodiscard]] GlobalPtr<void> chainedTryAllocate(std::size_t bytes, std::size_t alignment,
                                                 First* first, Rest*... rest) {
  GlobalPtr<void> ptr = first->allocate(bytes, alignment);
  if (ptr != nullptr) {
    return ptr;
  } else {
    return chainedTryAllocate(bytes, alignment, rest...);
  }
}

template <typename T>
void chainedTryDeallocate(GlobalPtr<void> p, std::size_t bytes, std::size_t alignment,
                          T* allocator) {
  allocator->deallocate(p, bytes, alignment);
}

template <typename First, typename... Rest>
void chainedTryDeallocate(GlobalPtr<void> p, std::size_t bytes, std::size_t alignment, First* first,
                          Rest*... rest) {
  if (first->pointerIsOwned(p) && First::supportsFree()) {
    first->deallocate(p, bytes, alignment);
  } else {
    chainedTryDeallocate(p, bytes, alignment, rest...);
  }
}

} // namespace

L2SPResourceRatioBreakdown::L2SPResourceRatioBreakdown(GlobalPtr<std::byte> bufferStart,
                                                       std::size_t bufferSize)
    : bucket8{0.2, 0, nullptr}, bucket16{0.3f, 0, nullptr}, bucket32{0.4, 0, nullptr} {
  // ensure all slabs are aligned with slab size alignment
  // empty bucket for consistent function calls
  MemoryBucket startBucket{0.0, 0, bufferStart};
  alignStartAndRound<SlabMemoryResource<8>::resourceSlabSize>(bucket8, startBucket, bufferStart,
                                                              bufferSize);
  alignStartAndRound<SlabMemoryResource<16>::resourceSlabSize>(bucket16, bucket8, bufferStart,
                                                               bufferSize);
  alignStartAndRound<SlabMemoryResource<32>::resourceSlabSize>(bucket32, bucket16, bufferStart,
                                                               bufferSize);

  // Free list alignment and computation
  initializeFreeListResourceBucket(freeList, bucket32, bufferStart, bufferSize);

  // Bump alignment and computation
  initializeBumpResourceBucket(bump, freeList, bufferStart, bufferSize);
}

MainMemoryResourceRatioBreakdown::MainMemoryResourceRatioBreakdown(GlobalPtr<std::byte> bufferStart,
                                                                   std::size_t bufferSize)
    : bucket8{0.006f, 0, nullptr},
      bucket16{0.006f, 0, nullptr},
      bucket32{0.006f, 0, nullptr},
      bucket64{0.063f, 0, nullptr},
      bucket128{0.031f, 0, nullptr} {
  // ensure all slabs are aligned with slab size alignment
  // empty bucket for consistent function calls
  MemoryBucket startBucket{0.0, 0, bufferStart};
  alignStartAndRound<SlabMemoryResource<8>::resourceSlabSize>(bucket8, startBucket, bufferStart,
                                                              bufferSize);
  alignStartAndRound<SlabMemoryResource<16>::resourceSlabSize>(bucket16, bucket8, bufferStart,
                                                               bufferSize);
  alignStartAndRound<SlabMemoryResource<32>::resourceSlabSize>(bucket32, bucket16, bufferStart,
                                                               bufferSize);
  alignStartAndRound<SlabMemoryResource<64>::resourceSlabSize>(bucket64, bucket32, bufferStart,
                                                               bufferSize);
  alignStartAndRound<SlabMemoryResource<128>::resourceSlabSize>(bucket128, bucket64, bufferStart,
                                                                bufferSize);

  // Free list alignment and computation
  initializeFreeListResourceBucket(freeList, bucket128, bufferStart, bufferSize);

  // Bump alignment and computation
  initializeBumpResourceBucket(bump, freeList, bufferStart, bufferSize);
}

L2SPResource::L2SPResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize)
    : m_breakdown(bufferStart, bufferSize),
      m_bucket8(m_breakdown.bucket8.start, m_breakdown.bucket8.bytes),
      m_bucket16(m_breakdown.bucket16.start, m_breakdown.bucket16.bytes),
      m_bucket32(m_breakdown.bucket32.start, m_breakdown.bucket32.bytes),
      m_freeList(m_breakdown.freeList.start, m_breakdown.freeList.bytes),
      m_bump(m_breakdown.bump.start, m_breakdown.bump.bytes) {}

GlobalPtr<void> L2SPResource::allocate(std::size_t bytes, std::size_t alignment) {
  GlobalPtr<void> result{nullptr};
  if (bytes <= m_bucket8.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket8, &m_bucket16, &m_bucket32);
  } else if (bytes <= m_bucket16.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket16, &m_bucket32);
  } else if (bytes <= m_bucket32.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket32);
  }
  if (result == nullptr) {
    auto roundedAllocation = std::max(bytes, minimumBumpAllocation);
    // try allocating from the bump resource
    result = m_bump.allocate(roundedAllocation);
    if (result == nullptr) {
      // try allocating from the free-list resource
      result = m_freeList.allocate(roundedAllocation);
    }
  }

  return result;
}

void L2SPResource::deallocate(GlobalPtr<void> p, std::size_t bytes, std::size_t alignment) {
  if (bytes <= m_bucket8.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket8, &m_bucket16, &m_bucket32);
  } else if (bytes <= m_bucket16.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket16, &m_bucket32);
  } else if (bytes <= m_bucket32.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket32);
  }

  if (m_bump.pointerIsOwned(p)) {
    // any bump resource allocation is at least minimumBumpAllocation bytes
    auto roundedAllocation = std::max(bytes, minimumBumpAllocation);
    // the bump resource does not support deallocation but
    // the free-list resource can manage freed memory blocks from the bump resource.
    m_freeList.registerFreedBlock(p, roundedAllocation);
  }
}

bool L2SPResource::operator==(const L2SPResource& rhs) const noexcept {
  return (m_bucket8 == rhs.m_bucket8) && (m_bucket16 == rhs.m_bucket16) &&
         (m_bucket32 == rhs.m_bucket32) && (m_bump == rhs.m_bump) && (m_freeList == rhs.m_freeList);
}

bool L2SPResource::operator!=(const L2SPResource& rhs) const noexcept {
  return !(*this == rhs);
}

MainMemoryResource::MainMemoryResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize)
    : m_breakdown(bufferStart, bufferSize),
      m_bucket8(m_breakdown.bucket8.start, m_breakdown.bucket8.bytes),
      m_bucket16(m_breakdown.bucket16.start, m_breakdown.bucket16.bytes),
      m_bucket32(m_breakdown.bucket32.start, m_breakdown.bucket32.bytes),
      m_bucket64(m_breakdown.bucket64.start, m_breakdown.bucket64.bytes),
      m_bucket128(m_breakdown.bucket128.start, m_breakdown.bucket128.bytes),
      m_freeList(m_breakdown.freeList.start, m_breakdown.freeList.bytes),
      m_bump(m_breakdown.bump.start, m_breakdown.bump.bytes) {}

GlobalPtr<void> MainMemoryResource::allocate(std::size_t bytes, std::size_t alignment) {
  GlobalPtr<void> result{nullptr};
  if (bytes <= m_bucket8.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket8, &m_bucket16, &m_bucket32, &m_bucket64,
                                &m_bucket128);
  } else if (bytes <= m_bucket16.resourceSlabSize) {
    result =
        chainedTryAllocate(bytes, alignment, &m_bucket16, &m_bucket32, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket32.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket32, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket64.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket128.resourceSlabSize) {
    result = chainedTryAllocate(bytes, alignment, &m_bucket128);
  }

  if (result == nullptr) {
    auto roundedAllocation = std::max(bytes, minimumBumpAllocation);
    // try allocating from the bump resource
    result = m_bump.allocate(roundedAllocation);
    if (result == nullptr) {
      // try allocating from the free-list resource
      result = m_freeList.allocate(roundedAllocation);
    }
  }
  return result;
}

void MainMemoryResource::deallocate(GlobalPtr<void> p, std::size_t bytes, std::size_t alignment) {
  if (bytes <= m_bucket8.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket8, &m_bucket16, &m_bucket32, &m_bucket64,
                         &m_bucket128);
  } else if (bytes <= m_bucket16.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket16, &m_bucket32, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket32.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket32, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket64.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket64, &m_bucket128);
  } else if (bytes <= m_bucket128.resourceSlabSize) {
    chainedTryDeallocate(p, bytes, alignment, &m_bucket128);
  }

  if (m_bump.pointerIsOwned(p)) {
    // any bump resource allocation is at least minimumBumpAllocation bytes
    auto roundedAllocation = std::max(bytes, minimumBumpAllocation);
    // the bump resource does not support deallocation but
    // the free-list resource can manage freed memory blocks from the bump resource.
    m_freeList.registerFreedBlock(p, roundedAllocation);
  }
}

bool MainMemoryResource::operator==(const MainMemoryResource& rhs) const noexcept {
  return (m_bucket8 == rhs.m_bucket8) && (m_bucket16 == rhs.m_bucket16) &&
         (m_bucket32 == rhs.m_bucket32) && (m_bucket64 == rhs.m_bucket64) &&
         (m_bucket128 == rhs.m_bucket128) && (m_bump == rhs.m_bump) &&
         (m_freeList == rhs.m_freeList);
}

bool MainMemoryResource::operator!=(const MainMemoryResource& rhs) const noexcept {
  return !(*this == rhs);
}

namespace {

// Fields of the MainMemoryResource and L2SPResource objects are not tracked by
// simulation (SST) however the internal state of all resources is tracked
NodeSpecificStorage<MainMemoryResource*> mainMemoryResource;
// TODO(ypapadop-amd): #66 this needs to change to support multiple pods
NodeSpecificStorage<L2SPResource*> l2SPResource;

} // namespace

void initMemoryResources() {
  if (getCurrentCore() == CoreIndex(0, 0) && getCurrentThread() == ThreadIndex(0)) {
    // construct L2SP memory resource for the PXN by its Core-0 Hart-0
    auto [baseAddress, byteCount] = detail::getMemoryStartAndSize(MemoryType::L2SP);
    l2SPResource = new L2SPResource(baseAddress, byteCount);
  } else if (isOnCP()) {
    // construct main memory resource for the PXN by its CP
    auto [baseAddress, byteCount] = detail::getMemoryStartAndSize(MemoryType::Main);
    mainMemoryResource = new MainMemoryResource(baseAddress, byteCount);
  }
}

void finalizeMemoryResources() {
  if (getCurrentCore() == CoreIndex(0, 0) && getCurrentThread() == ThreadIndex(0)) {
    // destroy L2SP memory resource for the PXN by its Core-0 Hart-0
    delete static_cast<L2SPResource*>(l2SPResource);
  } else if (isOnCP()) {
    // destroy main memory resource for the PXN by its CP
    delete static_cast<MainMemoryResource*>(mainMemoryResource);
  }
}

L2SPResource* getDefaultL2SPResource() noexcept {
  return static_cast<L2SPResource*>(l2SPResource);
}

MainMemoryResource* getDefaultMainMemoryResource() noexcept {
  return static_cast<MainMemoryResource*>(mainMemoryResource);
}

} // namespace pando
