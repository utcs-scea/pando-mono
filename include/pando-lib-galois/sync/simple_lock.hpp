// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_SIMPLE_LOCK_HPP_
#define PANDO_LIB_GALOIS_SYNC_SIMPLE_LOCK_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/atomic.hpp>

namespace galois {
/**
 * @brief This is a termination detection mechanism that is used for detecting nested parallelism
 */
class SimpleLock {
  using LockState = std::uint64_t;
  enum class State : LockState { IsUnlocked = 0, IsLocked = 1 };

  /**
   * @brief This is a pointer to the state used by everyone
   */
  pando::GlobalPtr<LockState> m_state = nullptr;

public:
  // construct
  constexpr SimpleLock() noexcept = default;
  // movable
  constexpr SimpleLock(SimpleLock&&) noexcept = default;
  // copyable
  constexpr SimpleLock(const SimpleLock&) noexcept = default;
  // destruct
  ~SimpleLock() = default;

  // copy-assignable
  constexpr SimpleLock& operator=(const SimpleLock&) noexcept = default;
  // move-assignable
  constexpr SimpleLock& operator=(SimpleLock&&) noexcept = default;

  /**
   * @brief initializes the SimpleLock
   *
   * @param[in] place        the location the counter should be allocated at
   * @param[in] memoryType   the type of memory the waitgroup should be allocated in
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] pando::Status initialize(pando::Place place, pando::MemoryType memoryType) {
    m_state = PANDO_EXPECT_RETURN(pando::allocateMemory<LockState>(1, place, memoryType));
    *m_state = static_cast<LockState>(State::IsUnlocked);
    pando::atomicThreadFence(std::memory_order_release);
    return pando::Status::Success;
  }

  /**
   * @brief initializes the SimpleLock
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] pando::Status initialize() {
    return initialize(pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  /**
   * @brief deinitializes the SimpleLock and frees associated memory
   *
   * @warning not threadsafe but designed to be idempotent.
   */
  void deinitialize() {
    if (m_state != nullptr) {
      pando::deallocateMemory(m_state, 1);
      m_state = nullptr;
    }
  }

  /**
   * @brief Acquire the lock or block until the lock is acquired.
   */
  void lock() {
    while (!try_lock()) {}
  }

  /**
   * @brief Attempt to acquire the lock and returns immediately on success or failure.
   *
   * @return true If the lock was successfully acquired.
   * @return false If the lock acquire attempt failed.
   */
  bool try_lock() {
    constexpr auto success = std::memory_order_acquire;
    constexpr auto failure = std::memory_order_relaxed;
    auto expected = static_cast<LockState>(State::IsUnlocked);
    auto desired = static_cast<LockState>(State::IsLocked);
    return pando::atomicCompareExchange(m_state, pando::GlobalPtr<LockState>(&expected),
                                        pando::GlobalPtr<const LockState>(&desired), success,
                                        failure);
  }
  /**
   * @brief Release the lock.
   */
  void unlock() {
    auto desiredValue = static_cast<LockState>(State::IsUnlocked);
    pando::atomicStore(m_state, desiredValue, std::memory_order_release);
  }
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_SYNC_SIMPLE_LOCK_HPP_
