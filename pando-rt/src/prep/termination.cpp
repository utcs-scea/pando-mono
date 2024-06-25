// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/execution/termination.hpp"
#include "pando-rt/sync/atomic.hpp"

namespace pando {

// Per-PXN (main memory) variables
NodeSpecificStorage<std::int64_t> taskCreatedCount;
NodeSpecificStorage<std::int64_t> taskFinishedCount;

void TerminationDetection::increaseTasksCreated(Place place, std::int64_t n) noexcept {
  (void) place;
  atomicIncrement(&taskCreatedCount, n, std::memory_order_relaxed);
}

void TerminationDetection::increaseTasksFinished(std::int64_t n) noexcept {
  atomicIncrement(&taskFinishedCount, n, std::memory_order_relaxed);
}

TerminationDetection::TaskCounts TerminationDetection::getTaskCounts() noexcept {
  auto finished = atomicLoad(&taskFinishedCount, std::memory_order_seq_cst);
  auto created = atomicLoad(&taskCreatedCount, std::memory_order_seq_cst);
  return TaskCounts{created, finished};
}

} // namespace