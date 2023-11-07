// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
#define PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_

#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_utilities.hpp>
#include <pando-rt/sync/atomic.hpp>

namespace pando {
/**
 * @brief This is a termination detection mechanism that is used for detecting nested parallelism
 */
class WaitGroup {
  ///@brief This is a pointer to the counter used by everyone
  GlobalPtr<std::uint64_t> m_count = nullptr;

public:
  class HandleType {
    GlobalPtr<std::uint64_t> m_count = nullptr;
    explicit HandleType(GlobalPtr<std::uint64_t> countPtr) : m_count(countPtr) {}
    friend WaitGroup;

  public:
    HandleType() : m_count(nullptr) {}
    HandleType(const HandleType&) = default;
    HandleType& operator=(const HandleType&) = default;

    HandleType(HandleType&&) = default;
    HandleType& operator=(HandleType&&) = default;

    /**
     * @brief adds a number of more items to arrive at the barrier before release
     *
     * @param[in] delta the amount of things to wait on
     */
    void add(std::uint32_t delta) {
      atomicFetchAdd(m_count, static_cast<std::uint64_t>(delta), std::memory_order_release);
    }
    /**
     * @brief adds to the barrier to represent one more done to wait on
     */
    void addOne() {
      atomicFetchAdd(m_count, static_cast<std::uint64_t>(1), std::memory_order_release);
    }
    /**
     * @brief Signals that one of the things in the WaitGroup has completed.
     */
    void arrive() {
      atomicFetchSub(m_count, static_cast<std::uint64_t>(1), std::memory_order_release);
    }
  };

  WaitGroup() : m_count(nullptr) {}

  WaitGroup(const WaitGroup&) = delete;
  WaitGroup& operator=(const WaitGroup&) = delete;

  WaitGroup(WaitGroup&&) = delete;
  WaitGroup& operator=(WaitGroup&&) = delete;

  /**
   * @brief initializes the WaitGroup
   *
   * @param[in] initialCount the count that the WaitGroup should start with
   * @param[in] place        the location the counter should be allocated at
   * @param[in] memoryType   the type of memory the waitgroup should be allocated in
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] Status initialize(std::uint32_t initialCount, Place place, MemoryType memoryType) {
    Status status;
    std::tie(m_count, status) = allocateMemory<std::uint64_t>(place, 1, memoryType);
    if (status == Status::Success) {
      *m_count = initialCount;
      atomicThreadFence(std::memory_order_release);
    }
    return status;
  }

  /**
   * @brief initializes the WaitGroup
   *
   * @param[in] initialCount the count that the WaitGroup should start with
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] Status initialize(std::uint32_t initialCount) {
    return initialize(initialCount, getCurrentPlace(), MemoryType::Main);
  }

  /**
   * @brief deinitializes the waitgroup and frees associated memory
   *
   * @warning not threadsafe but designed to be idempotent.
   */
  void deinitialize() {
    if (m_count != nullptr) {
      deallocateMemory(m_count, 1);
      m_count = nullptr;
    }
  }

  HandleType getHandle() {
    return HandleType{m_count};
  }

  /**
   * @brief Waits until the number of items to wait on is zero.
   */
  void wait() {
    waitUntil([this] {
      const bool ready = *m_count == static_cast<std::uint64_t>(0);
      return ready;
    });
    atomicThreadFence(std::memory_order_acquire);
  }
};
} // namespace pando
#endif // PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
