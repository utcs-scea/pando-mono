// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_ADDRESS_MAP_HPP_
#define PANDO_RT_MEMORY_ADDRESS_MAP_HPP_

#include "../utility/bit_manip.hpp"

namespace pando {

/**
 * @brief Describes the address map of PANDO global address pointers.
 *
 * @ingroup ROOT
 */
constexpr inline struct {
  /// @brief Memory type bits.
  BitRange memoryType{58, 64};

  /// @brief L1SP bits.
  struct {
    BitRange nodeIndex{44, 58};
    BitRange podY{28, 31};
    BitRange podX{25, 28};
    BitRange coreY{22, 25};
    BitRange coreX{19, 22};
    BitRange global{18, 19};
    BitRange offset{0, 18};
  } L1SP;

  /// @brief L2SP bits.
  struct {
    BitRange nodeIndex{44, 58};
    BitRange podY{28, 31};
    BitRange podX{25, 28};
    BitRange offset{0, 25};
  } L2SP;

  /// @brief Main memory bits.
  struct {
    BitRange nodeIndex{44, 58};
    BitRange offset{0, 44};
  } main;
} addressMap;

// Node index bits need to match for L1SP, L2SP and main memory
static_assert((addressMap.L1SP.nodeIndex == addressMap.L2SP.nodeIndex) &&
              (addressMap.L1SP.nodeIndex == addressMap.main.nodeIndex));

// Pod index bits need to match for L1SP and L2SP
static_assert((addressMap.L1SP.podX == addressMap.L2SP.podX) &&
              (addressMap.L1SP.podY == addressMap.L2SP.podY));

} // namespace pando

#endif // PANDO_RT_MEMORY_ADDRESS_MAP_HPP_
