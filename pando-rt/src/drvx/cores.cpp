// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "cores.hpp"

#include <stdlib.h> // required for setenv

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "cp.hpp"
#include "drvx.hpp"
#include "index.hpp"
#include "log.hpp"
#include "status.hpp"

namespace pando {

namespace {

enum class CoreState : std::int8_t { Stopped = 0, Idle, Ready };

// Convert CoreState to underlying integral type
constexpr auto operator+(CoreState e) noexcept {
  return static_cast<std::underlying_type_t<CoreState>>(e);
}

// Per-core (L1SP) variables
Drvx::StaticL1SP<Cores::TaskQueue*> coreQueue;

} // namespace

void Cores::initializeQueues() {
  // One hart per core does all the initialization. Use CAS to choose some/any hart for this
  // purpose.
  if (DrvAPI::atomicCompareExchangeCoreState(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x, +CoreState::Stopped, +CoreState::Idle) == +CoreState::Stopped) {
    // initialize core object
    coreQueue = new TaskQueue;

    // indicate that core initialization is complete and state is ready
    DrvAPI::setCoreState(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x, +CoreState::Ready);

    // CP waits for this variable to equal total number of cores in the PXN
    DrvAPI::atomicIncrementPxnCoresInitialized(Drvx::getCurrentNode().id, 1);
  }

  // Other harts just wait until state is ready
  while (DrvAPI::getCoreState(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x) != +CoreState::Ready) {
    hartYield(1000);
  }
}

void Cores::finalizeQueues() {
  // One hart per core does all the finalization. Use CAS to choose some/any hart for this
  // purpose.
  DrvAPI::atomicIncrementCoreHartsDone(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x, 1);

  if (DrvAPI::atomicCompareExchangeCoreState(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x, +CoreState::Ready, +CoreState::Idle) == +CoreState::Ready) {
    // wait for all harts on this core to be done
    while (DrvAPI::getCoreHartsDone(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x) != Drvx::getThreadDims().id) {
      hartYield(1000);
    }

    DrvAPI::atomicIncrementPodCoresFinalized(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, 1);

    // wait for all cores on this pod to be finalized to ensure no work stealing before deleting the queue
    while (DrvAPI::getPodCoresFinalized(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x) != Drvx::getCoreDims().x) {
      hartYield(1000);
    }

    DrvAPI::setCoreState(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x, Drvx::getCurrentCore().x, +CoreState::Stopped);

    TaskQueue* queue = coreQueue;
    delete queue;
  }

  // Other harts simply exit
}

Cores::CoreActiveFlag Cores::getCoreActiveFlag() noexcept {
  return CoreActiveFlag{};
}

bool Cores::CoreActiveFlag::operator*() const noexcept {
  do {
    hartYield(1000);
  } while ((DrvAPI::getGlobalCpsFinalized() != Drvx::getNodeDims().id) && (DrvAPI::getPodTasksRemaining(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x) == 0));

  if (DrvAPI::getGlobalCpsFinalized() != Drvx::getNodeDims().id) {
    return true;
  } else {
    return DrvAPI::getPodTasksRemaining(Drvx::getCurrentNode().id, Drvx::getCurrentPod().x) != 0;
  }
}

Cores::TaskQueue* Cores::getTaskQueue(Place place) noexcept {
  return *toNativeDrvPtr(coreQueue, place);
}

void Cores::finalize() {
}

void Cores::workStealing(std::optional<pando::Task>& task) {
  const auto thisPlace = getCurrentPlace();
  const auto coreDims = getCoreDims();

  // onyl steal from the neighbor core
  for(std::int8_t i = thisPlace.core.x + 1; i < thisPlace.core.x + 2; i++) {
    auto* otherQueue =  pando::Cores::getTaskQueue(pando::Place{thisPlace.node, thisPlace.pod, pando::CoreIndex(i % coreDims.x, 0)});
    if(otherQueue->getApproxSize() > STEAL_THRESH_HOLD_SIZE) {
      task = otherQueue->tryDequeue();
    }
  }
}

} // namespace pando
