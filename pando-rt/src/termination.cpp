// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/execution/termination.hpp"

#include "pando-rt/sync/atomic.hpp"

#if defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/drvx.hpp"
#endif // PANDO_RT_USE_BACKEND_DRVX

namespace pando {

// Per-PXN (main memory) variables
#ifdef PANDO_RT_USE_BACKEND_PREP
NodeSpecificStorage<std::int64_t> taskCreatedCount;
NodeSpecificStorage<std::int64_t> taskFinishedCount;
#endif // PANDO_RT_USE_BACKEND_PREP

void TerminationDetection::increaseTasksCreated(Place place, std::int64_t n) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  (void) place;
  atomicIncrement(&taskCreatedCount, n, std::memory_order_relaxed);
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::atomicIncrementPodTasksRemaining(place.node.id, place.pod.x, n);
#endif
}

void TerminationDetection::increaseTasksFinished(std::int64_t n) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  atomicIncrement(&taskFinishedCount, n, std::memory_order_relaxed);
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::atomicIncrementPodTasksRemaining(pando::getCurrentNode().id, pando::getCurrentPod().x, -1 * n);
#endif
}

#if defined(PANDO_RT_USE_BACKEND_PREP)
TerminationDetection::TaskCounts TerminationDetection::getTaskCounts() noexcept {
  auto finished = atomicLoad(&taskFinishedCount, std::memory_order_seq_cst);
  auto created = atomicLoad(&taskCreatedCount, std::memory_order_seq_cst);
  return TaskCounts{created, finished};
}
#endif

} // namespace