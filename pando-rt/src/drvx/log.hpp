// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_DRVX_LOG_HPP_
#define PANDO_RT_SRC_DRVX_LOG_HPP_

#include "spdlog/spdlog.h"

#include "pando-rt/index.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Logging support.
 *
 * @ingroup DRVX
 */
class Logger {
public:
  /**
   * @brief Initializes the logging subsystem.
   */
  [[nodiscard]] static Status initialize();
};

} // namespace pando

#endif // PANDO_RT_SRC_DRVX_LOG_HPP_
