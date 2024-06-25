// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/execution/termination.hpp"
#include "pando-rt/sync/atomic.hpp"

#include "drvx.hpp"

namespace pando {
void TerminationDetection::increaseTasksCreated(Place place, std::int64_t n) noexcept {
  DrvAPI::atomicIncrementPodTasksRemaining(place.node.id, place.pod.x, n);
}

void TerminationDetection::increaseTasksFinished(std::int64_t n) noexcept {
  DrvAPI::atomicIncrementPodTasksRemaining(pando::getCurrentNode().id, pando::getCurrentPod().x, -1 * n);
}

} // namespace