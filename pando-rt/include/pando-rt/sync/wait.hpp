// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SYNC_WAIT_HPP_
#define PANDO_RT_SYNC_WAIT_HPP_

#include <cstdint>
#include <tuple>

#include "../utility/function.hpp"
#include "../memory/global_ptr.hpp"
#include "export.h"
#include <pando-rt/tracing.hpp>

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

/**
 * @brief Waits until @p f returns @c true.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void waitUntil(const Function<bool()>& f);

/**
 * @brief Waits until the value of @p ptr becomes @p value.
 *
 * @ingroup ROOT
 */
template <typename T>
void monitorUntil(GlobalPtr<T> ptr, T value) {
#ifdef PANDO_RT_USE_BACKEND_PREP
    waitUntil([ptr, value] {
      const bool ready = *ptr == value;
      PANDO_MEM_STAT_WAIT_GROUP_ACCESS();
      return ready;
    });
#else

#if defined(PANDO_RT_BYPASS)
    if (getBypassFlag()) {
      waitUntil([ptr, value] {
        const bool ready = *ptr == value;
        return ready;
      });
    } else {
      DrvAPI::monitor_until<T>(ptr.address, value);
    }
#else
    DrvAPI::monitor_until<T>(ptr.address, value);
#endif

#endif
}

/**
 * @brief Waits until the value of @p ptr is no longer @p value.
 *
 * @ingroup ROOT
 */
template <typename T>
void monitorUntilNot(GlobalPtr<T> ptr, T value) {
#ifdef PANDO_RT_USE_BACKEND_PREP
    waitUntil([ptr, value] {
      const bool ready = *ptr != value;
      PANDO_MEM_STAT_WAIT_GROUP_ACCESS();
      return ready;
    });
#else

#if defined(PANDO_RT_BYPASS)
    if (getBypassFlag()) {
      waitUntil([ptr, value] {
        const bool ready = *ptr != value;
        return ready;
      });
    } else {
      DrvAPI::monitor_until_not<T>(ptr.address, value);
    }
#else
    DrvAPI::monitor_until_not<T>(ptr.address, value);
#endif

#endif
}

/**
 * @brief Waits for all tasks to finish executing.
 *
 * @note This is a collective operation and needs to be called by all nodes.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void waitAll();

PANDO_RT_EXPORT void endExecution();

} // namespace pando

#endif // PANDO_RT_SYNC_WAIT_HPP_
