// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_COMMON_MEMORY_RESOURCE_HPP_
#define PANDO_RT_MEMORY_COMMON_MEMORY_RESOURCE_HPP_

#include <cstdint>

#include "../sync/atomic.hpp"
#include "global_ptr.hpp"

namespace pando {
namespace detail {
/**
 * @brief Helper mutex wrapper that does not own underlying storage for the mutex.
 */
struct InplaceMutex {
  using MutexValueType = std::uint64_t;

private:
  enum class MutexState : MutexValueType { isUnlocked = 0, isLocked = 1 };

  /**
   * @brief Convert @p memoryType to the underlying integral type.
   */
  constexpr friend auto operator+(MutexState mutexState) noexcept {
    return static_cast<std::underlying_type_t<MutexState>>(mutexState);
  }

public:
  /**
   * @brief Initialize a mutex object state.
   *
   * @param mutexStatePtr A pointer to the mutex object state to initialize
   */
  static void initialize(GlobalPtr<MutexValueType> mutexStatePtr) {
    atomicStore(mutexStatePtr, +MutexState::isUnlocked, std::memory_order_release);
  }
  /**
   * @brief Lock a mutex object state.
   *
   * @param mutexStatePtr A pointer to the mutex object state to lock
   */
  static void lock(GlobalPtr<MutexValueType> mutexStatePtr) {
    bool resourceLocked{false};
    do {
      resourceLocked = tryLock(mutexStatePtr);
    } while (!resourceLocked);
  }

  /**
   * @brief Attempt locking a mutex object state.
   *
   * @param mutexStatePtr A pointer to the mutex object state to lock
   * @return @c true if successfully locked the resource and @c false otherwise.
   */
  static bool tryLock(GlobalPtr<MutexValueType> mutexStatePtr) {
    const auto desired = +MutexState::isLocked;
    const auto expected = +MutexState::isUnlocked;
    return atomicCompareExchange(mutexStatePtr, expected, desired);
  }

  /**
   * @brief Unlock a mutex object state.
   *
   * @param mutexStatePtr A pointer to the mutex object state to unlock
   */
  static void unlock(GlobalPtr<MutexValueType> mutexStatePtr) {
    atomicStore(mutexStatePtr, +MutexState::isUnlocked, std::memory_order_release);
  }
};

} // namespace detail

} // namespace pando
#endif // PANDO_RT_MEMORY_COMMON_MEMORY_RESOURCE_HPP_
