// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "drvx.hpp"

namespace pando {

// Yields to the next hart
void hartYield(int cycle) noexcept {
  DrvAPI::nop(cycle);
}

std::int64_t Drvx::getNumSystemCores() noexcept {
  return DrvAPI::numPXNs() * DrvAPI::numPXNPods() * DrvAPI::numPodCores();
}

std::int64_t Drvx::getNumPxnCores() noexcept {
  return DrvAPI::numPXNPods() * DrvAPI::numPodCores();
}

NodeIndex Drvx::getCurrentNode() noexcept {
  return NodeIndex(DrvAPI::myPXNId());
}

PodIndex Drvx::getCurrentPod() noexcept {
  if (!isOnCP()) {
    return PodIndex(DrvAPI::myPodId(), 0);
  } else {
    return anyPod;
  }
}

CoreIndex Drvx::getCurrentCore() noexcept {
  if (!isOnCP()) {
    return CoreIndex(DrvAPI::myCoreId(), 0);
  } else {
    return anyCore;
  }
}

NodeIndex Drvx::getNodeDims() noexcept {
  return NodeIndex(DrvAPI::numPXNs());
}

PodIndex Drvx::getPodDims() noexcept {
  return PodIndex(DrvAPI::numPXNPods(), 1);
}

CoreIndex Drvx::getCoreDims() noexcept {
  return CoreIndex(DrvAPI::numPodCores(), 1);
}

ThreadIndex Drvx::getCurrentThread() noexcept {
  if (!isOnCP()) {
    return ThreadIndex(DrvAPI::myThreadId());
  } else {
    return ThreadIndex(-1);
  }
}

ThreadIndex Drvx::getThreadDims() noexcept {
  return ThreadIndex(DrvAPI::numCoreThreads());
}

bool Drvx::isOnCP() noexcept {
  return DrvAPI::isCommandProcessor();
}

} // namespace pando
