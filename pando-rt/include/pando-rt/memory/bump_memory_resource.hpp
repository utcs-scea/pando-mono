// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_BUMP_MEMORY_RESOURCE_HPP_
#define PANDO_RT_MEMORY_BUMP_MEMORY_RESOURCE_HPP_

#include <cstddef>
#include <cstdint>

#include "../memory.hpp"
#include "../stdlib.hpp"
#include "../sync/atomic.hpp"
#include "common_memory_resource.hpp"
#include "global_ptr.hpp"

namespace pando {

/**
 * @brief A fixed-size bump memory resource.
 *
 * @tparam MinimumAlignment The minimum alignment that the bump allocator should satisfy.
 *
 * @ingroup ROOT
 */
template <std::uint32_t MinimumAlignment>
class BumpMemoryResource {
private:
  using MutexValueType = detail::InplaceMutex::MutexValueType;

  GlobalPtr<std::byte> m_buffer; // The start of the buffer managed by the  memory resource. The
                                 // pointer is an invariant and lives in registers.

  GlobalPtr<std::size_t> m_curOffset; // The last unused offset in the buffer. The location of this
                                      // pointer lives in a predefined location (first 8 bytes).

  GlobalPtr<MutexValueType> m_mutex; // The mutex is global state that must be accessible by all
                                     // cores  and live in predefined place (the
                                     // `sizeof(MutexValueType)` bytes  after the `m_curOffset`).

  std::size_t m_capacity; // The bytes capacity of the resource. The capacity is a constant number
                          // that lives in registers.

public:
  /**
   * @brief Construct a new Bump Memory Resource object
   *
   * @param bufferStart Start of the buffer that the resource should use.
   * @param bufferSize The buffer size in bytes.
   */
  BumpMemoryResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize) {
    // align start for storage of currentOffset
    auto voidCurrentStart = static_cast<GlobalPtr<void>>(bufferStart);
    m_capacity = bufferSize;
    auto alignedCurrentStart =
        align(alignof(std::size_t), sizeof(std::size_t), voidCurrentStart, m_capacity);
    if (alignedCurrentStart == nullptr) {
      PANDO_ABORT("Insufficient space to store metadata");
    }
    m_curOffset = static_cast<GlobalPtr<std::size_t>>(alignedCurrentStart);
    m_capacity -= sizeof(std::size_t);

    // align start for storage of mutex
    voidCurrentStart = static_cast<GlobalPtr<std::byte>>(voidCurrentStart) + sizeof(std::size_t);
    alignedCurrentStart =
        align(alignof(MutexValueType), sizeof(MutexValueType), voidCurrentStart, m_capacity);
    if (alignedCurrentStart == nullptr) {
      PANDO_ABORT("Insufficient space to store metadata");
    }
    m_mutex = static_cast<GlobalPtr<MutexValueType>>(alignedCurrentStart);
    m_capacity -= sizeof(MutexValueType);

    m_buffer = static_cast<GlobalPtr<std::byte>>(voidCurrentStart) + sizeof(MutexValueType);

    // initialize global resource state values
    atomicStore(m_curOffset, 0, std::memory_order_relaxed);
    detail::InplaceMutex::initialize(m_mutex);
  }

  /**
   * @brief Destroy the Free List Memory Resource object
   *
   */
  ~BumpMemoryResource() = default;

  /**
   * @brief Allocates `bytes`
   *
   * @note Allocate does not respect alignment requests
   *
   * @param bytes The requested number of bytes
   * @return A pointer to the allocated memory or `nullptr` if the allocation failed.
   */
  [[nodiscard]] GlobalPtr<void> allocate(std::size_t bytes,
                                         std::size_t = alignof(std::max_align_t)) {
    detail::InplaceMutex::lock(m_mutex);

    auto allocOffset = *m_curOffset;
    auto allocBase = getBase();
    GlobalPtr<void> allocationPointer = allocBase + allocOffset;

    // The Bump resource must be compatible with the alignment requirement of the FreeList allocator
    // and hence we must align requests here
    auto availableBytes = m_capacity - allocOffset;
    auto alignedPointer = align(MinimumAlignment, bytes, allocationPointer, availableBytes);
    if (alignedPointer != nullptr) {
      *m_curOffset = m_capacity - availableBytes + bytes;
    }

    detail::InplaceMutex::unlock(m_mutex);

    return alignedPointer;
  }

  /**
   * @brief Deallocates a pointer
   *
   * @note Deallocation in a bump memory resource is a no-op
   */
  void deallocate(GlobalPtr<void>, std::size_t, std::size_t = alignof(std::max_align_t)) {
    return;
  }

  /**
   * @brief Equality comparison between two bump memory resources
   *
   * @param rhs A different bump memory resource
   * @return @c true If the two resources are the same
   * @return @c false If the two resources are different
   */
  bool operator==(const BumpMemoryResource& rhs) const noexcept {
    return (m_buffer == rhs.m_buffer);
  }

  /**
   * @brief Inequality comparison between two bump memory resources
   *
   * @param rhs A different bump memory resource
   * @return @c true If the two resources are not the same
   * @return @c false If the two resources are the same
   */
  bool operator!=(const BumpMemoryResource& rhs) const noexcept {
    return !(*this == rhs);
  }

  /**
   * @brief Checks if this memory resource owns the pointer
   *
   * @param p Pointer to check ownership for
   * @return @c true If the memory resource owns the pointer
   * @return @c false If the memory resource does not own the pointer
   */
  bool pointerIsOwned(GlobalPtr<const void> p) const {
    auto pointer = static_cast<GlobalPtr<const std::byte>>(p);
    auto bufferStart = m_buffer;
    auto bufferEnd = bufferStart + m_capacity;
    return (pointer >= bufferStart) && (pointer < bufferEnd);
  }

  /**
   * @brief Checks if the memory resource can deallocate memory
   *
   * @return @c true If the resource can free memory
   * @return @c false If the resource does not free memory i.e., all calls for `deallocate(p)` are
   * no-ops.
   */
  static constexpr bool supportsFree() {
    return false;
  }

  /**
   * @brief Computes the metadata size required by the resource.
   *
   * @return number of bytes for the memory resource.
   */
  static constexpr std::size_t computeMetadataSize() {
    return sizeof(std::size_t) + sizeof(MutexValueType);
  }

private:
  /**
   * @brief Get the Base pointer of the bump allocator buffer
   *
   * @return A global pointer to the user addressable buffer
   */
  GlobalPtr<std::byte> getBase() const noexcept {
    return m_buffer;
  }
};

} // namespace pando

#endif //  PANDO_RT_MEMORY_BUMP_MEMORY_RESOURCE_HPP_
