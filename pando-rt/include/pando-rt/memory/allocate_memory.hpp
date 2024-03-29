// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_ALLOCATE_MEMORY_HPP_
#define PANDO_RT_MEMORY_ALLOCATE_MEMORY_HPP_

#include <cstdint>

#include "../execution/execute_on.hpp"
#include "../execution/execute_on_wait.hpp"
#include "../locality.hpp"
#include "../memory/global_ptr.hpp"
#include "../status.hpp"
#include "../stdlib.hpp"
#include "../sync/future.hpp"
#include "../utility/expected.hpp"
#include "memory_type.hpp"

namespace pando {

namespace detail {

/**
 * @brief Allocates `n` bytes of uninitialized storage in the specified memory.
 *
 * @param[in] size       number of bytes to allocate
 * @param[in] memoryType memory in which the allocation should take place in
 *
 * @return pointer to the newly allocated memory or `nullptr` if the allocation failed or the memory
 *         is not accessible from the calling place
 *
 * @ingroup ROOT
 */
[[nodiscard]] GlobalPtr<void> allocateMemoryImpl(std::uint64_t size, MemoryType memoryType);

/**
 * @brief Deallocates the memory @p p points to.
 *
 * @param[in] p    pointer to memory to deallocate
 * @param[in] size number of bytes to deallocate
 *
 * @ingroup ROOT
 */
void deallocateMemoryImpl(GlobalPtr<void> p, std::uint64_t size);

} // namespace detail

/**
 * @brief Allocates `n * sizeof(T)` bytes of uninitialized storage memory in a specific place and
 *        memory.
 *
 * @tparam T object type to allocate memory for
 *
 * @note This is a blocking function.
 *
 * @param[in] place      place where the allocation should occur
 * @param[in] n          number of objects to allocate memory for
 * @param[in] memoryType memory in which the allocation should take place in
 *
 * @ingroup ROOT
 */
template <typename T>
[[nodiscard]] Expected<GlobalPtr<T>> allocateMemory(std::uint64_t n, Place place,
                                                    MemoryType memoryType) {
  const auto numBytes = n * sizeof(T);

  GlobalPtr<void> ptr;

  switch (memoryType) {
    case MemoryType::Main: {
      // CP / harts have direct access to main memory on their node
      if ((place.node == anyNode) || (place.node == getCurrentNode())) {
        ptr = detail::allocateMemoryImpl(numBytes, MemoryType::Main);
      } else {
        auto ret = executeOnWait(place, &detail::allocateMemoryImpl, numBytes, MemoryType::Main);
        if (!ret.hasValue()) {
          return ret.error();
        }
        ptr = ret.value();
      }
    } break;

    case MemoryType::L2SP: {
      // only harts on the same pod as the requested L2SP have direct access
      const auto thisPlace = getCurrentPlace();
      if (!isOnCP() && ((place.node == anyNode) || (thisPlace.node == place.node)) &&
          (thisPlace.pod == place.pod)) {
        ptr = detail::allocateMemoryImpl(numBytes, MemoryType::L2SP);
      } else {
        auto ret = executeOnWait(place, &detail::allocateMemoryImpl, numBytes, MemoryType::L2SP);
        if (!ret.hasValue()) {
          return ret.error();
        }
        ptr = ret.value();
      }
    } break;

    case MemoryType::L1SP:
    default:
      return Status::InvalidValue;
  }

  if (ptr == nullptr) {
    return Status::BadAlloc;
  } else {
    return static_cast<GlobalPtr<T>>(ptr);
  }
}

/**
 * @brief Allocates `n * sizeof(T)` bytes of uninitialized storage memory in a specific place and
 *        memory asynchronously.
 *
 * @tparam T object type to allocate memory for
 *
 * @param[out] promise    where to store the pointer to newly allocated space and signal that
 *                        allocation finished
 * @param[in]  place      place where the allocation should occur
 * @param[in]  n          number of objects to allocate memory for
 * @param[in]  memoryType memory in which the allocation should take place in
 *
 * @ingroup ROOT
 */
template <typename T>
[[nodiscard]] Status allocateMemory(PtrPromise<T> promise, std::uint64_t n, Place place,
                                    MemoryType memoryType) {
  auto allocateF = +[](PtrPromise<T> promise, std::uint64_t n, MemoryType memoryType) {
    auto ptr = detail::allocateMemoryImpl(sizeof(T) * n, memoryType);
    if (ptr == nullptr) {
      promise.setFailure();
    } else {
      promise.setValue(static_cast<GlobalPtr<T>>(ptr));
    }
  };
  return executeOn(place, allocateF, promise, n, memoryType);
}

/**
 * @brief Deallocates memory previously allocated with `allocateMemory`.
 *
 * @tparam T object type to deallocate memory for
 *
 * @param[in] p pointer to memory to deallocate
 * @param[in] n number of objects to @p p points to
 *
 * @ingroup ROOT
 */
template <typename T>
void deallocateMemory(GlobalPtr<T> p, std::uint64_t n) {
  if (p == nullptr) {
    return;
  }

  const auto numBytes = n * sizeof(T);
  const auto place = pando::localityOf(p);
  const auto memoryType = memoryTypeOf(p);

  switch (memoryType) {
    case MemoryType::Main: {
      // CP / harts have direct access to main memory on their node
      if (place.node == getCurrentNode()) {
        detail::deallocateMemoryImpl(p, numBytes);
      } else {
        if (executeOn(place, &detail::deallocateMemoryImpl, static_cast<GlobalPtr<void>>(p),
                      numBytes) != Status::Success) {
          PANDO_ABORT("Deallocation failed");
        }
      }
    } break;

    case MemoryType::L2SP: {
      // only harts on the same pod as the requested L2SP have direct access
      const auto thisPlace = getCurrentPlace();
      if (!isOnCP() && (thisPlace.node == place.node) && (thisPlace.pod == place.pod)) {
        detail::deallocateMemoryImpl(p, numBytes);
      } else {
        if (executeOn(place, &detail::deallocateMemoryImpl, static_cast<GlobalPtr<void>>(p),
                      numBytes) != Status::Success) {
          PANDO_ABORT("Deallocation failed");
        }
      }
    } break;

    case MemoryType::L1SP:
    default:
      PANDO_ABORT("Invalid pointer to deallocate");
      break;
  }
}

} // namespace pando

#endif // PANDO_RT_MEMORY_ALLOCATE_MEMORY_HPP_
