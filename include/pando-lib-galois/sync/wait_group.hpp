// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
#define PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_utilities.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/atomic.hpp>
#include <pando-rt/sync/notification.hpp>

static void addLocal(pando::GlobalPtr<std::int64_t> countPtr, std::uint32_t delta,
                     pando::NotificationHandle handle) {
  pando::atomicFetchAdd(countPtr, static_cast<std::int64_t>(delta), std::memory_order_release);
  handle.notify();
}

static void subLocalNoNotify(pando::GlobalPtr<std::int64_t> countPtr, std::uint32_t delta) {
  pando::atomicFetchSub(countPtr, static_cast<std::int64_t>(delta), std::memory_order_release);
}

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
      if (isSubsetOf(pando::getCurrentPlace(), pando::localityOf(m_count))) {
        pando::atomicFetchAdd(m_count, static_cast<std::int64_t>(delta), std::memory_order_release);
      } else {
        bool notifier;
        pando::Notification notify;

        if (pando::getCurrentPlace().core == pando::anyCore) {
          PANDO_CHECK(notify.init());
        } else {
          PANDO_CHECK(notify.init(&notifier));
        }

        PANDO_CHECK(pando::executeOn(pando::localityOf(m_count), &addLocal, m_count, delta,
                                     notify.getHandle()));
        notify.wait();
      }
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
      if (pando::isSubsetOf(pando::getCurrentPlace(), pando::localityOf(m_count))) {
        pando::atomicFetchSub(m_count, static_cast<std::int64_t>(1), std::memory_order_release);
      } else {
        pando::atomicThreadFence(std::memory_order_release);
        PANDO_CHECK(pando::executeOn(pando::localityOf(m_count), &subLocalNoNotify, m_count,
                                     static_cast<std::uint32_t>(1)));
      }
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
    pando::waitUntil([this] {
      const bool ready = *m_count <= static_cast<std::int64_t>(0);
      return ready;
    });
    pando::atomicThreadFence(std::memory_order_acquire);
    if (*m_count < static_cast<std::int64_t>(0)) {
      return pando::Status::Error;
    }
    return pando::Status::Success;
  }
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_SYNC_WAIT_GROUP_HPP_
