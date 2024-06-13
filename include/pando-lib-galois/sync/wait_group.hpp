// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
#define PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/atomic.hpp>
#include <pando-rt/sync/notification.hpp>
#include <pando-rt/tracing.hpp>

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "DrvAPIMemory.hpp"
#endif

namespace galois {
/**
 * @brief This is a termination detection mechanism that is used for detecting nested parallelism
 */
class WaitGroup {
  ///@brief This is a pointer to the counter used by everyone
  pando::GlobalPtr<std::int64_t> m_count = nullptr;

public:
  class HandleType {
    pando::GlobalPtr<std::int64_t> m_count = nullptr;
    explicit HandleType(pando::GlobalPtr<std::int64_t> countPtr) : m_count(countPtr) {}
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
      pando::atomicFetchAdd(m_count, static_cast<std::int64_t>(delta), std::memory_order_release);
    }
    /**
     * @brief adds to the barrier to represent one more done to wait on
     */
    void addOne() {
      add(static_cast<std::uint32_t>(1));
    }
    /**
     * @brief Signals that one of the things in the WaitGroup has completed.
     */
    void done() {
      pando::atomicDecrement(m_count, static_cast<std::int64_t>(1), std::memory_order_release);
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
  [[nodiscard]] pando::Status initialize(std::uint32_t initialCount, pando::Place place,
                                         pando::MemoryType memoryType) {
    const auto expected = pando::allocateMemory<std::int64_t>(1, place, memoryType);
    if (!expected.hasValue()) {
      return expected.error();
    }
    m_count = expected.value();
    *m_count = static_cast<std::int64_t>(initialCount);
    pando::atomicThreadFence(std::memory_order_release);
    return pando::Status::Success;
  }

  /**
   * @brief initializes the WaitGroup
   *
   * @param[in] initialCount the count that the WaitGroup should start with
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] pando::Status initialize(std::uint32_t initialCount) {
    return initialize(initialCount, pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  /**
   * @brief deinitializes the waitgroup and frees associated memory
   *
   * @warning not threadsafe but designed to be idempotent.
   */
  void deinitialize() {
    if (m_count != nullptr) {
      pando::deallocateMemory(m_count, 1);
      m_count = nullptr;
    }
  }

  HandleType getHandle() {
    return HandleType{m_count};
  }

  /**
   * @brief Waits until the number of items to wait on is zero.
   */
  [[nodiscard]] pando::Status wait() {
#ifdef PANDO_RT_USE_BACKEND_PREP
    pando::waitUntil([this] {
      const bool ready = *m_count <= static_cast<std::int64_t>(0);
      PANDO_MEM_STAT_WAIT_GROUP_ACCESS();
      return ready;
    });
#else

#if defined(PANDO_RT_BYPASS)
    if (getBypassFlag()) {
      pando::waitUntil([this] {
        const bool ready = *m_count <= static_cast<std::int64_t>(0);
        return ready;
      });
    } else {
      DrvAPI::monitor_until<int64_t>(m_count.address, static_cast<std::int64_t>(0));
    }
#else
    DrvAPI::monitor_until<int64_t>(m_count.address, static_cast<std::int64_t>(0));
#endif

#endif
    pando::atomicThreadFence(std::memory_order_acquire);
    PANDO_MEM_STAT_NEW_PHASE();
    if (*m_count < static_cast<std::int64_t>(0)) {
      return pando::Status::Error;
    }
    return pando::Status::Success;
  }
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
