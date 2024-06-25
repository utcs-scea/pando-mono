// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_DRVX_CP_HPP_
#define PANDO_RT_SRC_DRVX_CP_HPP_

#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Command Processor component.
 *
 * @ingroup DRVX
 */
class CommandProcessor {
public:
  /**
   * @brief Initializes all the CPs
   */
  [[nodiscard]] static Status initialize();

  /**
   * @brief Barrier for all CPs.
   */
  static void barrier();

  /**
   * @brief Finalizes the CP.
   */
  static void finalize();
};

} // namespace pando

#endif // PANDO_RT_SRC_DRVX_CP_HPP_
