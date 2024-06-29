// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_UTILITY_MATH_HPP_
#define PANDO_RT_UTILITY_MATH_HPP_

#include <cstdint>

namespace pando {

/**
 * @brief Get the floor of the log2 of @p i.
 *
 * @warning @i 0 is not a valid input.
 *
 * @param[in] i input number
 *
 * @ingroup ROOT
 */
constexpr std::uint8_t log2floor(std::uint64_t i) {
  return static_cast<std::uint8_t>(8 * sizeof(std::uint64_t) - __builtin_clzll(i) - 1);
}

/**
 * @brief Get the power of 2 larger than @p i.
 *
 * @param[in] i input number
 *
 * @ingroup ROOT
 */
constexpr std::uint64_t up2(std::uint64_t i) noexcept {
  auto log2 = log2floor(i);
  return static_cast<std::uint64_t>(1) << (log2 + 1);
}

} // namespace pando

#endif // PANDO_RT_UTILITY_MATH_HPP_
