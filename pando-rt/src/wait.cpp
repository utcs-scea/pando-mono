// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/sync/wait.hpp"

#include "pando-rt/locality.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "pando-rt/specific_storage.hpp"
#include "pando-rt/stdlib.hpp"
#include "pando-rt/sync/atomic.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#include "prep/nodes.hpp"
#ifdef PANDO_RT_ENABLE_MEM_STAT
#include "prep/memtrace_stat.hpp"
#endif
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cp.hpp"
#include "drvx/drvx.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

namespace {

// Per-PXN (main memory) variables
NodeSpecificStorage<std::int64_t> taskCreatedCount;
NodeSpecificStorage<std::int64_t> taskFinishedCount;
#ifdef PANDO_RT_USE_BACKEND_DRVX
NodeSpecificStorage<std::int64_t> partialTasksCount;
#endif // PANDO_RT_USE_BACKEND_DRVX

// Performs an allreduce operation. This is a collective operation by CPs across all PXNs.
std::int64_t allreduce(std::int64_t partialValue) {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Nodes::allreduce(partialValue);

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  // set partial value to PXN-local static
  partialTasksCount = partialValue;

  // barrier
  CommandProcessor::barrier();

  // reduce in each PXN by adding up partial values across all PXNs
  std::int64_t value = 0;
  for (auto nodeIdx = 0; nodeIdx < getNodeDims().id; ++nodeIdx) {
    value += *(partialTasksCount.getPointerAt(NodeIndex(nodeIdx)));
  }

  // barrier again to wait for all PXNs to complete reduction
  CommandProcessor::barrier();

  return value;

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace

void TerminationDetection::increaseTasksCreated(std::int64_t n) noexcept {
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

void waitUntil(const Function<bool()>& f) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  hartYieldUntil([&f] {
    return f();
  });

#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  // DrvX CP is not modeled as a separate thread, so all cores (CP and PH cores) yield while busy
  // waiting.
  do {
    hartYield();
  } while (f() == false);

#else

#error "Not implemented"

#endif
}

void waitAll() {
  if (!isOnCP()) {
    PANDO_ABORT("Can only be called from the CP");
  }

  // Termination Detection: this algorithm exits iff all the created tasks in the system have been
  // executed.
  //
  // The first allreduce is designed to fail to guarantee that all nodes are in the termination
  // detection, i.e., act as a barrier, and get a baseline of created tasks. In the best case,
  // the algorithm will have to do two (2) allreduces.
  //
  // On each node we keep track of how many tasks where created
  // (TerminationDetection::getTaskCounts()::created) and how many where executed
  // (TerminationDetection::getTaskCounts()::finished). increasing). Each node contributes the
  // different between the two in the allreduce, so that if the sum of (created - finished) tasks
  // in the system is 0, then quiescence has been reached.
  //
  // The number of newly created tasks is added to the partial differences so that if a task has
  // been created to a node that has already contributed a value to the allreduce, the termination
  // detection round will fail to correctly account for that task.

  auto prevCreatedTasks = atomicLoad(&taskCreatedCount, std::memory_order_relaxed);
  auto partialPendingTasks = prevCreatedTasks; // don't count finished to fail the first time
  auto newTasksCreated = prevCreatedTasks;
  while (true) {
    const auto globalNewTasksCreated = allreduce(newTasksCreated);
    const auto globalPendingTasks = allreduce(partialPendingTasks);
    if (globalPendingTasks == 0 && globalNewTasksCreated == 0) {
      break;
    }

    const auto partialTaskCounts = TerminationDetection::getTaskCounts();
    newTasksCreated = partialTaskCounts.created - prevCreatedTasks;
    partialPendingTasks = (partialTaskCounts.created - partialTaskCounts.finished);
    prevCreatedTasks = partialTaskCounts.created;
  }

#ifdef PANDO_RT_ENABLE_MEM_STAT
  MemTraceStat::writePhase();
#endif
}

} // namespace pando
