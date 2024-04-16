// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "cp.hpp"

#include "cores.hpp"
#include "drvx.hpp"
#include "index.hpp"
#include "pando-rt/status.hpp"
#include "status.hpp"

namespace pando {

namespace {

// Per-PXN (main memory) variables
Drvx::StaticMainMem<std::int64_t> globalBarrierCounter;
Drvx::StaticMainMem<std::int64_t> pxnBarrierDone;
Drvx::StaticMainMem<std::int64_t> coresDone;

} // namespace

// Initialize logger and memory resources
[[nodiscard]] Status CommandProcessor::initialize() {
  // wait until all cores have initialized
  Cores::waitForCoresInit();

  // wait for all CP to finish initialization
  barrier();

  SPDLOG_INFO("CP started on PXN {} with {} cores", Drvx::getCurrentNode(), Drvx::getCoreDims());

  return Status::Success;
}

void CommandProcessor::barrier() {
  if (Drvx::isOnCP()) {
    // reset barrier on self PXN
    *toNativeDrvPointerOnDram(pxnBarrierDone, Drvx::getCurrentNode()) = 0u;

    // enter the global barrier by incrementing the global counter on PXN-0
    std::int64_t curBarrierCount =
        DrvAPI::atomic_add(toNativeDrvPointerOnDram(globalBarrierCounter, NodeIndex(0)), DrvAPI::phase_t::PHASE_OTHER, 1u);
    if (curBarrierCount == Drvx::getNodeDims().id - 1) {
      // last PXN to reach barrier; reset global barrier counter and signal to other PXNs that we
      // are done
      *toNativeDrvPointerOnDram(globalBarrierCounter, NodeIndex(0)) = 0u;
      for (std::int16_t i = 0; i < Drvx::getNodeDims().id; ++i) {
        *toNativeDrvPointerOnDram(pxnBarrierDone, NodeIndex(i)) = 1;
      }
    } else {
      // other PXNs are yet to reach the barrier; wait for the last PXN to reach the barrier to
      // notify this PXN.
      while (*toNativeDrvPointerOnDram(pxnBarrierDone, Drvx::getCurrentNode()) != 1u) {
        hartYield();
      }
    }
    SPDLOG_INFO("Barrier completed on PXN {}", Drvx::getCurrentNode());
  }
}

void CommandProcessor::signalCoresDone() {
  DrvAPI::atomic_add(toNativeDrvPointerOnDram(coresDone, NodeIndex(0)), DrvAPI::phase_t::PHASE_OTHER, 1u);
}

/// Wait for all cores on all PXNs to be done.
static void waitForCoresDone() {
  while (*toNativeDrvPointerOnDram(coresDone, NodeIndex(0)) != Drvx::getNumSystemCores()) {
    hartYield();
  }
}

void CommandProcessor::finalize() {
  Cores::finalize();

  // wait for all cores to complete their hart loop
  waitForCoresDone();
  SPDLOG_INFO("CP finalized on PXN {}", Drvx::getCurrentNode());
}

} // namespace pando
