// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_FREELIST_MEMORY_RESOURCE_HPP_
#define PANDO_RT_MEMORY_FREELIST_MEMORY_RESOURCE_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>

#include "../stdlib.hpp"
#include "../sync/atomic.hpp"
#include "common_memory_resource.hpp"
#include "global_ptr.hpp"

namespace pando {

/**
 * @brief A free-list memory resource that adhere to the standard memory resource abstraction
 *
 * @note The free list memory resource does not own memory; it only manages freed memory blocks
 *       added to the list on deallocate calls.
 *
 * @ingroup ROOT
 */
class FreeListMemoryResource {
  struct FreeListNode {
    GlobalPtr<FreeListNode> next{nullptr};
    GlobalPtr<FreeListNode> previous{nullptr};
    std::uint64_t blockSize{0};
  };

  GlobalPtr<FreeListNode> m_head; // A pointer to the head of linked list. This pointer is a
                                  // sentinel that its next member points to the first available
                                  // memory block.

  using MutexValueType = detail::InplaceMutex::MutexValueType;

  GlobalPtr<MutexValueType> m_mutex; // The mutex is global state that must be accessible by all
                                     // cores and live in predefined place in the memory buffer
                                     // managed by the resource.

public:
  /**
   * @brief Construct a new free-list memory resource object.

   * @param bufferStart Start of the buffer that the resource should use.
   * @param bufferSize The buffer size in bytes. The buffer size must be greater than
   *                   the value returned by `computeMetadataSize`.
   */
  FreeListMemoryResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize) {
    // align start for storage of mutex
    auto voidCurrentStart = static_cast<GlobalPtr<void>>(bufferStart);
    auto alignedCurrentStart =
        align(alignof(MutexValueType), sizeof(MutexValueType), voidCurrentStart, bufferSize);
    if (alignedCurrentStart == nullptr) {
      PANDO_ABORT("Insufficient space to store metadata");
    }
    m_mutex = static_cast<GlobalPtr<MutexValueType>>(alignedCurrentStart);
    bufferSize -= sizeof(MutexValueType);

    // store sentinel free list node
    if (bufferSize < sizeof(FreeListNode)) {
      PANDO_ABORT("Insufficient space to store metadata");
    }
    GlobalPtr<void> currentStart =
        static_cast<GlobalPtr<std::byte>>(alignedCurrentStart) + sizeof(MutexValueType);
    currentStart = align(alignof(FreeListNode), sizeof(FreeListNode), currentStart, bufferSize);
    if (currentStart == nullptr) {
      PANDO_ABORT("Insufficient space to store metadata");
    }
    m_head = globalPtrReinterpretCast<GlobalPtr<FreeListNode>>(currentStart);

    // initialize global resource state values
    *m_head = FreeListNode{nullptr, nullptr, 0};
    detail::InplaceMutex::initialize(m_mutex);
  }

  /**
   * @brief Destroy the Free List Memory Resource object
   */
  ~FreeListMemoryResource() = default;

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
    GlobalPtr<void> result(nullptr);
    if (m_head->next != nullptr) {
      auto bestMatchingBlock = findBestMatchingBlock(bytes);
      // The head node is a sentinel node and can't be allocated
      if (bestMatchingBlock != m_head) {
        removeBlockFromList(bestMatchingBlock);
        result = bestMatchingBlock;
      }
    }
    detail::InplaceMutex::unlock(m_mutex);
    return result;
  }

  /**
   * @brief  Deallocates the storage pointed to by a pointer.
   *
   * @note The storage pointed to by the pointer is used by the FreeListMemoryResource to store
   * header bits managed by the allocator.
   *
   * @param p Pointer to the storage to be deallocated
   * @param bytes The number of bytes that the pointer points to. `bytes` must be greater or equal
   * to `sizeof(FreeListNode)`
   */
  void deallocate(GlobalPtr<void> p, std::size_t bytes, std::size_t = alignof(std::max_align_t)) {
    registerFreedBlock(p, bytes);
  }

  /**
   * @brief Equality comparison between two free-list memory resources
   *
   * @param rhs A different free-list memory resource
   * @return @c true If the two resources are the same
   * @return @c false If the two resources are different
   */

  bool operator==(const FreeListMemoryResource& rhs) const noexcept {
    return (m_head == rhs.m_head);
  }

  /**
   * @brief Inequality comparison between two free-list memory resources
   *
   * @param rhs A different free-list memory resource
   * @return @c true If the two resources are not the same
   * @return @c false If the two resources are the same
   */
  bool operator!=(const FreeListMemoryResource& rhs) const noexcept {
    return !(*this == rhs);
  }

  /**
   * @brief Checks if this memory resource owns the pointer.
   *
   * @note The free list memory resource does not own any memory. This call always returns
   * `false`.
   *
   * @return @c false always
   */
  bool pointerIsOwned(GlobalPtr<const void>) const {
    return false;
  }

  /**
   * @brief Checks if the memory resource can deallocate memory
   *
   * @return @c true If the resource can free memory
   * @return @c false If the resource does not free memory i.e., all calls for `deallocate(p)` are
   * no-ops.
   */
  static constexpr bool supportsFree() {
    return true;
  }

  /**
   * @brief Computes the metadata size required by the resource.
   *
   * @return constexpr std::size_t number of bytes for the memory resource.
   */
  static constexpr std::size_t computeMetadataSize() {
    return sizeof(FreeListNode) + sizeof(MutexValueType);
  }

  /**
   * @brief Finds the minimum allocation size that the resource supports.
   *
   * @return Minimum number of bytes that the resource can manage.
   */
  static constexpr std::size_t minimumAllowableAllocationSize() {
    return sizeof(FreeListNode);
  }

  /**
   * @brief Register a memory block with the free list resource.
   *
   * @note The pointer @p p must be aligned to at least @c alignof(FreeListNode) and points to a
   * buffer of at least the same number of bytes returned by minimumAllowableAllocationSize.
   *
   * @param p A pointer to the buffer to register.
   * @param bytes The number of bytes that the pointer @p p points to.
   */
  void registerFreedBlock(GlobalPtr<void> p, std::size_t bytes) {
    if ((p == nullptr) || (bytes < sizeof(FreeListNode))) {
      PANDO_ABORT("Insufficient space to store node metadata");
    }

    // Alignment is guaranteed to be satisfied if the Bump resource pointers are the ones passed
    // here (which is the case).
    const auto requiredAlignment = alignof(FreeListNode);
    const auto pointerSatisfyAlignment =
        (globalPtrReinterpretCast<std::uintptr_t>(p) % requiredAlignment) == 0;

    if (!pointerSatisfyAlignment) {
      PANDO_ABORT("FreeList required pointer alignment is not maintained");
    }

    detail::InplaceMutex::lock(m_mutex);
    auto newBlockPtr = static_cast<GlobalPtr<FreeListNode>>(p);
    addBlock(newBlockPtr, bytes);
    detail::InplaceMutex::unlock(m_mutex);
  }

private:
  GlobalPtr<FreeListNode> findBestMatchingBlock(std::size_t bytes) const {
    auto current = m_head;
    auto bestNode = current;
    auto bestDiff = std::numeric_limits<std::size_t>::max();
    while (current != nullptr) {
      auto blockSize = current->blockSize;
      if (blockSize > bytes) {
        auto diff = blockSize - bytes;
        if (diff < bestDiff) {
          bestDiff = diff;
          bestNode = current;
        }
      } else if (bytes == blockSize) {
        return current;
      }
      current = current->next;
    }
    return bestNode;
  }
  void removeBlockFromList(GlobalPtr<const FreeListNode> blockPtr) {
    const FreeListNode block = *blockPtr;
    auto previous = block.previous;
    auto next = block.next;

    if (previous != nullptr) {
      previous->next = next;
    }
    if (next != nullptr) {
      next->previous = previous;
    }
  }
  void addBlock(GlobalPtr<FreeListNode> nodePtr, std::size_t bytes) {
    // insert the new memory block at the beginning of the linked list
    auto nextNode = m_head->next;
    if (nextNode != nullptr) {
      nextNode->previous = nodePtr;
    }

    nodePtr->previous = m_head;
    nodePtr->next = nextNode;
    nodePtr->blockSize = bytes;

    m_head->next = nodePtr;
  }
};

} // namespace pando

#endif // PANDO_RT_MEMORY_FREELIST_MEMORY_RESOURCE_HPP_
