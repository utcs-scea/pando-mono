// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_DRVX_CORES_HPP_
#define PANDO_RT_SRC_DRVX_CORES_HPP_

#include <cstddef>
#include <tuple>

#include "../start.hpp"
#include "../queue.hpp"
#include "pando-rt/execution/task.hpp"
#include "pando-rt/index.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Cores component that models PandoHammer cores and pods.
 *
 * @ingroup DRVX
 */
class Cores {
public:
  using TaskQueue = Queue<Task>;

  /**
   * @brief Flag to check if the core is active.
   */
  struct CoreActiveFlag {
    bool operator*() const noexcept;
  };

  /**
   * @brief Initializes queues.
   */
  static void initializeQueues();

  /**
   * @brief Finalizes queues.
   */
  static void finalizeQueues();

  /**
   * @brief Finalizes the cores subsystem.
   */
  static void finalize();

  /**
   * @brief Returns the queue associated with place @p place.
   */
  [[nodiscard]] static TaskQueue* getTaskQueue(Place place) noexcept;

  /**
   * @brief Returns a flag to check if the core is active.
   */
  static CoreActiveFlag getCoreActiveFlag() noexcept;

  /**
   * @brief Steals work from other cores.
   */
  static void workStealing(std::optional<pando::Task>& task);
};

} // namespace pando

#endif // PANDO_RT_SRC_DRVX_CORES_HPP_
