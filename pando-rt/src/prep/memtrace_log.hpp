// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_MEMTRACE_LOG_HPP_
#define PANDO_RT_SRC_PREP_MEMTRACE_LOG_HPP_

#include <string_view>

#include "pando-rt/index.hpp"
#include "pando-rt/memory/global_ptr_fwd.hpp"

namespace pando {

/**
 * @brief Memory trace logging support.
 *
 * @ingroup PREP
 */
class MemTraceLogger {
public:
  /**
   * @brief Logs memory operations using spdlog at INFO level.
   */
  static void log(std::string_view const op, pando::NodeIndex source, pando::NodeIndex dest,
                  std::size_t size = 0, const void* localBuffer = nullptr,
                  GlobalAddress globalAddress = 0);
};
} // namespace pando

#endif // PANDO_RT_SRC_PREP_MEMTRACE_LOG_HPP_
