// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_MEMORY_TYPE_HPP_
#define PANDO_RT_MEMORY_MEMORY_TYPE_HPP_

#include <cstdint>
#include <type_traits>

namespace pando {

/**
 * @brief Memory types.
 *
 * @ingroup ROOT
 */
enum class MemoryType : std::uint8_t {
  /// @brief L1SP memory
  L1SP = 0b000000,
  /// @brief L2SP memory
  L2SP = 0b000001,
  /// @brief Main memory
  Main = 0b000010,
  Unknown = 0b111111,
};

/**
 * @brief Convert @p memoryType to the underlying integral type.
 *
 * @ingroup ROOT
 */
constexpr auto operator+(MemoryType memoryType) noexcept {
  return static_cast<std::underlying_type_t<MemoryType>>(memoryType);
}

} // namespace pando

#endif // PANDO_RT_MEMORY_MEMORY_TYPE_HPP_
