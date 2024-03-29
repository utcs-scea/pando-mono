// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_HART_CONTEXT_HPP_
#define PANDO_RT_SRC_PREP_HART_CONTEXT_HPP_

/**
 * @defgroup PREP PREP
 *
 * PREP (PANDO Runtime Exercise Platform) is a software emulator of the PANDO hardware using
 * [Qthreads] for PANDOHammer emulation and [GASNet-EX] for communication and remote memory
 * operations support.
 *
 * [Qthreads]:  https://www.sandia.gov/qthreads/ "Qthreads"
 * [GASNet-EX]: https://gasnet.lbl.gov/          "GASNet-Ex"
 * @{
 */
/**@}*/

#include <cstddef>
#include <cstdint>

#include "qthread/qthread.h"

#include "hart_context_fwd.hpp"
#include "pando-rt/index.hpp"
#include "status.hpp"

namespace pando {

/// @ingroup PREP
class ComputeCore;

/**
 * @brief Context in which an emulated PandoHammer hart (hardware thread) executes.
 *
 * This class is required for bookkeeping of hart execution during software emulation.
 *
 * @ingroup PREP
 */
struct HartContext {
  /// @brief Hart ID type
  using IDType = ThreadIndex;
  /// @brief Hart entry function
  using EntryFunction = int (*)(int, char*[]);
  /// @brief Function result type
  using ResultType = aligned_t;

  /// @brief Result for joining a qthread
  ResultType result{};
  /// @brief Hart ID
  IDType id{-1};
  /// @brief Hart stack range; all variables allocated on the stack will be within this range
  struct {
    std::byte* begin;
    std::byte* end;
  } stackAddressRange{};
  /// @brief Core this hart belongs to
  ComputeCore* core{};
  /// @brief Hart entry function
  EntryFunction entry{};

  constexpr HartContext(IDType id, ComputeCore* core, EntryFunction entry) noexcept
      : id{id}, core{core}, entry(entry) {}

  /**
   * @brief Returns the offset of @p p from the start of the stack of this hart.
   */
  constexpr std::ptrdiff_t getStackOffset(const void* p) const noexcept {
    const auto ptr = static_cast<const std::byte*>(p);
    if (ptr >= stackAddressRange.end) {
      return -1;
    }
    return ptr - stackAddressRange.begin;
  }

  /**
   * @brief Returns the address in offset @p offset from this hart's base stack address.
   */
  constexpr void* getStackAddress(std::size_t offset) const noexcept {
    const auto ptr = stackAddressRange.begin + offset;
    if (ptr >= stackAddressRange.end) {
      return nullptr;
    }
    return ptr;
  }
};

/**
 * @brief Sets the context of the current hart to @p context.
 *
 * @ingroup PREP
 */
Status hartContextSet(HartContext* context) noexcept;

/**
 * @brief Reset the context of the current hart.
 *
 * @ingroup PREP
 */
void hartContextReset() noexcept;

/**
 * @brief Returns the context of the current hart or @c nullptr if the function was not called in a
 *        hart.
 *
 * @ingroup PREP
 */
HartContext* hartContextGet() noexcept;

/**
 * @brief Yields to the next hart.
 *
 * @param[in] context context of the hart that is yielding
 *
 * @ingroup PREP
 */
void hartYield(HartContext& context) noexcept;

/**
 * @brief Yields to the next hart.
 *
 * @warning This function should be called when only one call is needed and the context is not
 *          known.
 *
 * @ingroup PREP
 */
void hartYield() noexcept;

} // namespace pando

#endif // PANDO_RT_SRC_PREP_HART_CONTEXT_HPP_
