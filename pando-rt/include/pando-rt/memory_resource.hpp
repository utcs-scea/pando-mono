// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_RESOURCE_HPP_
#define PANDO_RT_MEMORY_RESOURCE_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "export.h"
#include "memory/bump_memory_resource.hpp"
#include "memory/freelist_memory_resource.hpp"
#include "memory/global_ptr.hpp"
#include "memory/slab_memory_resource.hpp"
#include "stddef.hpp"

namespace pando {

/**
 * @brief A description of a memory allocator bucket.
 *
 * @ingroup ROOT
 */
struct MemoryBucket {
  double ratio;
  std::size_t bytes;
  GlobalPtr<std::byte> start;
};

/**
 * @brief A description of how the main memory resource is broken down into buckets.
 *
 * @ingroup ROOT
 */
struct MainMemoryResourceRatioBreakdown {
  MemoryBucket bucket8;
  MemoryBucket bucket16;
  MemoryBucket bucket32;
  MemoryBucket bucket64;
  MemoryBucket bucket128;
  MemoryBucket freeList;
  MemoryBucket bump;

  /**
   * @brief Construct a new Main Memory Resource Ratio Breakdown object
   *
   * @param bufferStart A pointer to the start of the main memory buffer.
   * @param bufferSize The size of the main memory buffer.
   */
  MainMemoryResourceRatioBreakdown(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize);
};

/**
 * @brief A description of how the L2SP resource is broken down into buckets.
 *
 * @ingroup ROOT
 */
struct L2SPResourceRatioBreakdown {
  MemoryBucket bucket8;
  MemoryBucket bucket16;
  MemoryBucket bucket32;
  MemoryBucket freeList;
  MemoryBucket bump;

  /**
   * @brief Construct a new L2SPResourceRatioBreakdown object
   *
   * @param bufferStart A pointer to the start of the L2SP memory buffer.
   * @param bufferSize The size of the L2SP memory buffer.
   */
  L2SPResourceRatioBreakdown(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize);
};

/**
 * @brief A memory resource that manages PXN-wide allocations for the L2 scratchpad
 *
 * @ingroup ROOT
 */
class L2SPResource {
  L2SPResourceRatioBreakdown m_breakdown;
  SlabMemoryResource<8> m_bucket8;
  SlabMemoryResource<16> m_bucket16;
  SlabMemoryResource<32> m_bucket32;
  FreeListMemoryResource m_freeList;
  BumpMemoryResource<alignof(MaxAlignT)> m_bump;

  // The bump resource must be compatible with the free-list resource
  static constexpr auto minimumBumpAllocation =
      FreeListMemoryResource::minimumAllowableAllocationSize();

public:
  /**
   * @brief Construct a new L2SPResource object
   *
   * @param bufferStart Pointer to the region of memory that is managed by the resource.
   * @param bufferSize The size of the buffer that the resource will manage.
   */
  L2SPResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize);

  /**
   * @brief Copy constructor is deleted.
   */
  L2SPResource(const L2SPResource&) = delete;

  /**
   * @brief Move constructor is deleted.
   */
  L2SPResource(L2SPResource&&) = delete;

  /**
   * @brief Copy-assignment operator is deleted.
   */
  L2SPResource& operator=(const L2SPResource&) = delete;

  /**
   * @brief Move-assignment operator is deleted.
   */
  L2SPResource& operator=(L2SPResource&&) = delete;

  /**
   * @brief Main Memory Resource destructor is defaulted.
   *
   */
  ~L2SPResource() = default;

  /**
   * @brief Allocates `bytes`
   *
   * @note Allocate does not respect alignment requests
   *
   * @param bytes The requested number of bytes
   * @param alignment alignment of the memory to allocate
   * @return void* A pointer to the allocated memory or `nullptr` if the allocation failed.
   */
  [[nodiscard]] GlobalPtr<void> allocate(std::size_t bytes,
                                         std::size_t alignment = alignof(std::max_align_t));

  /**
   * @brief  Deallocates the storage pointed to by a pointer.
   *
   * @param p Pointer to the storage to be deallocated
   * @param bytes The number of bytes that the pointer points to
   * @param alignment The alignment used for the pointer `p`
   */
  void deallocate(GlobalPtr<void> p, std::size_t bytes,
                  std::size_t alignment = alignof(std::max_align_t));

  /**
   * @brief Equality comparison between two L2SP memory resources
   *
   * @param rhs A different L2SP memory resource
   * @return @c true If the two resources are the same
   * @return @c false If the two resources are different
   */

  bool operator==(const L2SPResource& rhs) const noexcept;

  /**
   * @brief Inequality comparison between two L2SP memory resources
   *
   * @param rhs A different L2SP memory resource
   * @return @c true If the two resources are not the same
   * @return @c false If the two resources are the same
   */
  bool operator!=(const L2SPResource& rhs) const noexcept;
};

/**
 * @brief A memory resource that manages PXN-wide allocations for the main memory
 *
 * @ingroup ROOT
 */
class MainMemoryResource {
  MainMemoryResourceRatioBreakdown m_breakdown;
  SlabMemoryResource<8> m_bucket8;
  SlabMemoryResource<16> m_bucket16;
  SlabMemoryResource<32> m_bucket32;
  SlabMemoryResource<64> m_bucket64;
  SlabMemoryResource<128> m_bucket128;
  FreeListMemoryResource m_freeList;
  BumpMemoryResource<alignof(MaxAlignT)> m_bump;

  // The bump resource must be compatible with the free-list resource
  static constexpr auto minimumBumpAllocation =
      FreeListMemoryResource::minimumAllowableAllocationSize();

public:
  /**
   * @brief Construct a new Main Memory Resource object
   *
   * @param bufferStart Pointer to the region of memory that is managed by the resource.
   * @param bufferSize The size of the buffer that the resource will manage.
   */
  MainMemoryResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize);

  /**
   * @brief Copy constructor is deleted.
   */
  MainMemoryResource(const MainMemoryResource&) = delete;

  /**
   * @brief Move constructor is deleted.
   */
  MainMemoryResource(MainMemoryResource&&) = delete;

  /**
   * @brief Copy-assignment operator is deleted.
   */
  MainMemoryResource& operator=(const MainMemoryResource&) = delete;

  /**
   * @brief Move-assignment operator is deleted.
   */
  MainMemoryResource& operator=(MainMemoryResource&&) = delete;

  /**
   * @brief Main Memory Resource destructor is defaulted.
   *
   */
  ~MainMemoryResource() = default;

  /**
   * @brief Allocates `bytes`
   *
   * @note Allocate does not respect alignment requests
   *
   * @param bytes The requested number of bytes
   * @param alignment alignment of the memory to allocate
   * @return void* A pointer to the allocated memory or `nullptr` if the allocation failed.
   */
  [[nodiscard]] GlobalPtr<void> allocate(std::size_t bytes,
                                         std::size_t alignment = alignof(std::max_align_t));
  /**
   * @brief  Deallocates the storage pointed to by a pointer.
   *
   * @param p Pointer to the storage to be deallocated
   * @param bytes The number of bytes that the pointer points to
   * @param alignment The alignment used for the pointer p
   */
  void deallocate(GlobalPtr<void> p, std::size_t bytes,
                  std::size_t alignment = alignof(std::max_align_t));
  /**
   * @brief Equality comparison between two main memory memory resources
   *
   * @param rhs A different main memory memory resource
   * @return @c true If the two resources are the same
   * @return @c false If the two resources are different
   */

  bool operator==(const MainMemoryResource& rhs) const noexcept;

  /**
   * @brief Inequality comparison between two main memory memory resources
   *
   * @param rhs A different main memory memory resource
   * @return @c true If the two resources are not the same
   * @return @c false If the two resources are the same
   */
  bool operator!=(const MainMemoryResource& rhs) const noexcept;
};

/**
 * @brief Gets the default L2 scratchpad memory resource pointer.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT L2SPResource* getDefaultL2SPResource() noexcept;

/**
 * @brief Gets the default main memory memory resource pointer.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT MainMemoryResource* getDefaultMainMemoryResource() noexcept;

} // namespace pando

#endif // PANDO_RT_MEMORY_RESOURCE_HPP_
