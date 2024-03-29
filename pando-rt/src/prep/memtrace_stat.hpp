// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_MEMTRACE_STAT_HPP_
#define PANDO_RT_SRC_PREP_MEMTRACE_STAT_HPP_

#include <string_view>

#include "pando-rt/index.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Memory access statistics logging support.
 *
 * @ingroup PREP
 */
class MemTraceStat {
public:
  /**
   * @brief Initialize the log file per node
   */
  [[nodiscard]] static Status initialize(NodeIndex nodeIdx, NodeIndex nodeDims);

  /**
   * @brief Add the corresponding counters into the internal container.
   */
  static void add(std::string_view op, pando::NodeIndex other, std::size_t size = 0);

  /**
   * @brief Write memory stat counters as a new phase
   */
  static void writePhase();

  /**
   * @brief Start a new kernel and reset the phase counter
   */
  static void startKernel(std::string_view kernelName);

  /**
   *  Close the log file properly and do other cleaning if necessary
   */
  static void finalize();
};
} // namespace pando

#endif // PANDO_RT_SRC_PREP_MEMTRACE_STAT_HPP_
