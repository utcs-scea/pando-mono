// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_CONFIG_HPP_
#define PANDO_RT_SRC_PREP_CONFIG_HPP_

#include <cstddef>
#include <cstdint>

#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Emulated PANDO system configuration.
 *
 * @ingroup PREP
 */
class Config {
public:
  /**
   * @brief Configuration for an execution.
   */
  struct Instance {
    struct {
      std::uint32_t coreCount = 8;  // cores per pod
      std::uint32_t hartCount = 16; // harts per core (FGMT)
    } compute;
    struct {
      std::size_t l1SPHart = 0x2000;      // 8KiB L1 scratchpad per hart
      std::size_t l2SPPod = 0x2000000;    // 32MiB L2 scratchpad per pod
      std::size_t mainNode = 0x100000000; // 4GiB Main memory capacity per node
    } memory;
  };

public:
  /**
   * @brief Initializes the emulation configuration.
   */
  [[nodiscard]] static Status initialize();

  /**
   * @brief Returns the current config.
   */
  static const Instance& getCurrentConfig() noexcept;
};

} // namespace pando

#endif // PANDO_RT_SRC_PREP_CONFIG_HPP_
