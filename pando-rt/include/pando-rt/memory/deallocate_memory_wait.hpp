// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_DEALLOCATE_MEMORY_WAIT_HPP_
#define PANDO_RT_MEMORY_DEALLOCATE_MEMORY_WAIT_HPP_

#include <cstdint>

#include "../execution/execute_on.hpp"
#include "../execution/execute_on_wait.hpp"
#include "../locality.hpp"
#include "../memory/global_ptr.hpp"
#include "../status.hpp"
#include "../stdlib.hpp"
#include "../sync/future.hpp"
#include "../sync/wait_group.hpp"
#include "../utility/expected.hpp"
#include "memory_type.hpp"

namespace pando {

namespace detail {

/**
 * @brief Deallocates the memory @p p points to.
 *
 * @param[in] p    pointer to memory to deallocate
 * @param[in] size number of bytes to deallocate
 * @param[in] wgh  wait group that waits on deallocate
 *
 * @ingroup ROOT
 */
void deallocateMemoryWaitImpl(GlobalPtr<void> p, std::uint64_t size, pando::WaitGroup::HandleType wgh);

} // namespace detail
  
/**
 * @brief Deallocates memory previously allocated with `allocateMemory`.
 *
 * @tparam T object type to deallocate memory for
 *
 * @param[in] p pointer to memory to deallocate
 * @param[in] n number of objects to @p p points to
 * @param[in] wgh wait group that waits on deallocate
 *
 * @ingroup ROOT
 */
template <typename T>
void deallocateMemory(GlobalPtr<T> p, std::uint64_t n, pando::WaitGroup::HandleType wgh) {
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
        wgh.addOne();
        if (executeOn(place, &detail::deallocateMemoryWaitImpl, static_cast<GlobalPtr<void>>(p),
                      numBytes, wgh) != Status::Success) {
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

#endif // PANDO_RT_MEMORY_DEALLOCATE_MEMORY_WAIT_HPP_
