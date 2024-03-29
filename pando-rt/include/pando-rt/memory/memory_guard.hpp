// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#ifndef PANDO_RT_MEMORY_MEMORY_GUARD_HPP_
#define PANDO_RT_MEMORY_MEMORY_GUARD_HPP_

#include <cstdint>

#include "../memory_resource.hpp"

namespace pando {

/**
 * @brief Wrapper for allocating and deallocating using semantics similar to a @c std::lock_guard.
 */
template <typename T>
struct LocalStorageGuard {
  ///@brief the size of the allocation
  std::uint64_t size;
  ///@brief a reference to the pointer that needs to be filled
  GlobalPtr<T>& ptr;

  /**
   * @brief constructor designed to allocate memory used in an RAII fashion
   *
   * @param[in] ptr reference to the ptr that we want to fill
   * @param[in] size the number of elements to allocate
   *
   * @warning this structure is used for RAII memory management, use with care
   */
  explicit LocalStorageGuard(GlobalPtr<T>& ptr, uint64_t size) : size(size), ptr(ptr) {
    GlobalPtr<void> result = getDefaultMainMemoryResource()->allocate(sizeof(T) * size);
    ptr = static_cast<GlobalPtr<T>>(result);
  }

  /**
   * @brief destructor frees the pointer if it exists.
   */
  ~LocalStorageGuard() {
    if (ptr != nullptr) {
      getDefaultMainMemoryResource()->deallocate(ptr, sizeof(T) * size);
    }
  }

  LocalStorageGuard(const LocalStorageGuard&) = delete;
  LocalStorageGuard& operator=(const LocalStorageGuard&) = delete;
  LocalStorageGuard(LocalStorageGuard&&) = delete;
  LocalStorageGuard& operator=(LocalStorageGuard&&) = delete;
};

} // namespace pando

#endif // PANDO_RT_MEMORY_MEMORY_GUARD_HPP_
