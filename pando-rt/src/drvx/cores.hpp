// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_DRVX_CORES_HPP_
#define PANDO_RT_SRC_DRVX_CORES_HPP_

#include <cstddef>
#include <tuple>

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
   * @brief Initializes queues.
   */
  static void initializeQueues();

  /**
   * @brief Finalizes queues.
   */
  static void finalizeQueues();

  /**
   * @brief Waits for all cores.
   */
  static void waitForCoresInit();

  /**
   * @brief Returns the queue associated with place @p place.
   */
  [[nodiscard]] static TaskQueue* getTaskQueue(Place place) noexcept;
};

} // namespace pando

#endif // PANDO_RT_SRC_DRVX_CORES_HPP_
