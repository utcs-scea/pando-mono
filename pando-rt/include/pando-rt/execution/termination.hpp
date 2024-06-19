// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_TERMINATION_HPP_
#define PANDO_RT_EXECUTION_TERMINATION_HPP_

#include <cstdint>

#include "export.h"
#include "../specific_storage.hpp"
#include "../locality.hpp"

namespace pando {

// Per-PXN (main memory) variables
#ifdef PANDO_RT_USE_BACKEND_PREP
extern NodeSpecificStorage<std::int64_t> taskCreatedCount;
extern NodeSpecificStorage<std::int64_t> taskFinishedCount;
#endif // PANDO_RT_USE_BACKEND_PREP

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
  PANDO_RT_EXPORT static void increaseTasksCreated(Place place, std::int64_t n) noexcept;

  /**
   * @brief Increases the tasks finished count by @p n.
   */
  PANDO_RT_EXPORT static void increaseTasksFinished(std::int64_t n) noexcept;

#ifdef PANDO_RT_USE_BACKEND_PREP
  /**
   * @brief Returns the number of created and finished tasks.
   */
  PANDO_RT_EXPORT static TaskCounts getTaskCounts() noexcept;
#endif
};

} // namespace pando

#endif // PANDO_RT_EXECUTION_TERMINATION_HPP_