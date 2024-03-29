// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_HPP_
#define PANDO_RT_MEMORY_HPP_

#include <cstddef>
#include <type_traits>
#include <utility>

#include "export.h"
#include "locality.hpp"
#include "memory/global_ptr.hpp"
#include "stdlib.hpp"

namespace pando {

/**
 * @brief Aligns a pointer to a storage of size @p size within a buffer of size @p space.
 *
 * If alignment is possible, the function will update the pointer @p ptr, decrease the @p size with
 * the alignment overhead and return the aligned pointer. If the alignment is not possible,  @p ptr
 * and @p space will not be modified and the function will return @c nullptr.
 *
 * @param[in]    alignment requested alignment
 * @param[in]    size      size of storage that the pointer @p ptr points to
 * @param[inout] ptr       pointer to a buffer of at least @p space bytes
 * @param[inout] space     the size of buffer to operate on
 *
 * @return the aligned pointer if alignment succeeded and @c nullptr otherwise
 */
PANDO_RT_EXPORT GlobalPtr<void> align(std::size_t alignment, std::size_t size, GlobalPtr<void>& ptr,
                                      std::size_t& space);

/**
 * @brief Creates an object of type @p T at the address @p ptr.
 *
 * @ingroup ROOT
 */
template <typename T, typename... Args>
GlobalPtr<T> constructAt(GlobalPtr<T> ptr, Args&&... args) {
  static_assert(!std::is_array_v<T>);
  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == getCurrentNode()) {
    auto* p = detail::asNativePtr(ptr);
    ::new (p) T(std::forward<Args>(args)...);
  } else {
    if constexpr (std::is_trivially_copy_constructible_v<T> &&
                  std::is_trivially_copy_assignable_v<T>) {
      // assignment is ok because construction and assignment have no other side-effects
      *ptr = T(std::forward<Args>(args)...);
    } else {
      PANDO_ABORT("Not supported");
    }
  }
  return ptr;
}

/**
 * @brief Destroys the object pointed to by @p ptr.
 */
template <typename T>
void destroyAt(GlobalPtr<T> ptr) {
  static_assert(!std::is_array_v<T>);
  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == getCurrentNode()) {
    auto* p = detail::asNativePtr(ptr);
    p->~T();
  } else {
    PANDO_ABORT("Not supported");
  }
}

} // namespace pando

#endif // PANDO_RT_MEMORY_HPP_
