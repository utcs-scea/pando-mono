// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SYNC_MUTEX_HPP_
#define PANDO_RT_SYNC_MUTEX_HPP_

#include <cstdint>

#include "../memory/global_ptr.hpp"
#include "atomic.hpp"

namespace pando {
/**
 * @brief Mutex implementation.
 */
class Mutex {
  using MutexState = std::uint32_t;
  enum class State : MutexState { IsUnlocked = 0, IsLocked = 1 };
  alignas(4) MutexState m_state;
  /**
   * @brief Initialize the mutex state.
   */
  void initialize() {
    auto desiredValue = static_cast<MutexState>(State::IsUnlocked);
    atomicStore(GlobalPtr<MutexState>(&m_state), desiredValue,
                std::memory_order_release);
  }

public:
  /**
   * @brief Construct a new Mutex object.
   */
  Mutex() {
    initialize();
  }

  /**
   * @brief Destroy the Mutex object.
   */
  ~Mutex() = default;
  /**
   * @brief The mutex object is not copyable.
   */
  Mutex(const Mutex&) = delete;
  /**
   * @brief The mutex object is not copy-assignable.
   */
  Mutex& operator=(const Mutex&) = delete;
  /**
   * @brief The mutex object is not moveable.
   */
  Mutex(Mutex&&) = delete;
  /**
   * @brief The mutex object is not move-assignable.
   */
  Mutex& operator=(Mutex&&) = delete;

  /**
   * @brief Lock the mutex or block until the mutex is locked.
   */
  void lock() {
    while (!try_lock()) {}
  }
  /**
   * @brief Attempt to lock the mutex and returns immediately on success or failure.
   *
   * @return true If the mutex was successfully locked.
   * @return false If the mutex locking attempt failed.
   */
  bool try_lock() {
    constexpr auto success = std::memory_order_acquire;
    constexpr auto failure = std::memory_order_relaxed;
    auto expected = static_cast<MutexState>(State::IsUnlocked);
    auto desired = static_cast<MutexState>(State::IsLocked);
    return atomicCompareExchange(GlobalPtr<MutexState>(&m_state), expected, desired, success, failure);
  }
  /**
   * @brief Unlock the mutex.
   */
  void unlock() {
    auto desiredValue = static_cast<MutexState>(State::IsUnlocked);
    atomicStore(GlobalPtr<MutexState>(&m_state), desiredValue,
                std::memory_order_release);
  }
};
} // namespace pando

#endif // PANDO_RT_SYNC_MUTEX_HPP_
