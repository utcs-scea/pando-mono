// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SYNC_WAIT_HPP_
#define PANDO_RT_SYNC_WAIT_HPP_

#include <cstdint>
#include <tuple>

#include "../utility/function.hpp"
#include "export.h"

namespace pando {

/**
 * @brief Default termination detection mechanism.
 *
 * The default termination detection counts the number of created and finished asynchronous tasks.
 *
 * @ingroup ROOT
 */
class TerminationDetection {
public:
  /**
   * @brief Created and finished task counts.
   */
  struct TaskCounts {
    std::int64_t created{};
    std::int64_t finished{};
  };

  /**
   * @brief Increases the tasks created count by @p n.
   */
  PANDO_RT_EXPORT static void increaseTasksCreated(std::int64_t n) noexcept;

  /**
   * @brief Increases the tasks finished count by @p n.
   */
  PANDO_RT_EXPORT static void increaseTasksFinished(std::int64_t n) noexcept;

  /**
   * @brief Returns the number of created and finished tasks.
   */
  PANDO_RT_EXPORT static TaskCounts getTaskCounts() noexcept;
};

/**
 * @brief Waits until @p f returns @c true.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void waitUntil(const Function<bool()>& f);

/**
 * @brief Waits for all tasks to finish executing.
 *
 * @note This is a collective operation and needs to be called by all nodes.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void waitAll();

} // namespace pando

#endif // PANDO_RT_SYNC_WAIT_HPP_
