// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_UTILITY_BIT_MANIP_HPP_
#define PANDO_RT_UTILITY_BIT_MANIP_HPP_

#include <cstdint>
#include <type_traits>

namespace pando {

/**
 * @brief Describes a close-open range of bits `[lo, hi)`.
 *
 * @ingroup ROOT
 */
struct BitRange {
  std::uint32_t lo{};
  std::uint32_t hi{};

  /**
   * @brief Returns the number of bits in this range.
   */
  constexpr std::uint32_t width() const noexcept {
    return hi - lo;
  }
};

/// @ingroup ROOT
constexpr bool operator==(BitRange x, BitRange y) noexcept {
  return (x.lo == y.lo) && (x.hi == y.hi);
}

/// @ingroup ROOT
constexpr bool operator!=(BitRange x, BitRange y) noexcept {
  return !(x == y);
}

/**
 * @brief Returns the bits in the range @p bits from @p value.
 *
 * @ingroup ROOT
 */
template <typename UIntType>
constexpr UIntType readBits(UIntType value, BitRange bits) noexcept {
  static_assert(std::is_unsigned_v<UIntType>);
  const auto maskSize = bits.width();
  const auto mask = ~(~UIntType(0) << maskSize);
  return (value >> bits.lo) & mask;
}

/**
 * @brief Creates a mask with the bits in the range @p bit set to @p value.
 *
 * @ingroup ROOT
 */
template <typename UIntType>
constexpr UIntType createMask(BitRange bits, UIntType value) noexcept {
  static_assert(std::is_unsigned_v<UIntType>);
  const auto maskSize = bits.width();
  const auto mask = ~(~UIntType(0) << maskSize);
  return (value & mask) << bits.lo;
}

} // namespace pando

#endif // PANDO_RT_UTILITY_BIT_MANIP_HPP_
