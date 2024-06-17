// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "cp.hpp"

#include "cores.hpp"
#include "drvx.hpp"
#include "index.hpp"
#include "pando-rt/status.hpp"
#include "status.hpp"
#include "termination.hpp"

namespace pando {

namespace {

} // namespace

// Initialize logger and memory resources
[[nodiscard]] Status CommandProcessor::initialize() {
  // wait until all cores have initialized
  Cores::waitForCoresInitialized();

  // wait for all CP to finish initialization
  barrier();

  SPDLOG_INFO("CP started on PXN {} with {} cores", Drvx::getCurrentNode(), Drvx::getCoreDims());

  return Status::Success;
}

void CommandProcessor::barrier() {
  if (Drvx::isOnCP()) {
    // reset barrier on self PXN
    DrvAPI::resetPxnBarrierReached(Drvx::getCurrentNode().id);

    // enter the global barrier by incrementing the global counter on PXN-0
    std::int64_t curBarrierCount = DrvAPI::atomicIncrementNumPxnsDone(1);

    if (curBarrierCount == Drvx::getNodeDims().id - 1) {
      // last PXN to reach barrier; reset global barrier counter and signal to other PXNs that we
      // are done
      DrvAPI::resetNumPxnsDone();
      for (std::int64_t i = 0; i < Drvx::getNodeDims().id; ++i) {
        DrvAPI::setPxnBarrierReached(i);
      }
    } else {
      // other PXNs are yet to reach the barrier; wait for the last PXN to reach the barrier to
      // notify this PXN.
      while (!DrvAPI::testPxnBarrierReached(Drvx::getCurrentNode().id)) {
        hartYield();
      }
    }
    SPDLOG_INFO("Barrier completed on PXN {}", Drvx::getCurrentNode());
  }
}

void CommandProcessor::signalCoresDone() {
  DrvAPI::atomicIncrementNumCoresDone(Drvx::getCurrentNode().id, 1);
}

void CommandProcessor::waitForCoresDone() {
  while (DrvAPI::getNumCoresDone(Drvx::getCurrentNode().id) != Drvx::getNumPxnCores()) {
    hartYield();
  }
}

void CommandProcessor::signalCommandProcessorDone() {
  DrvAPI::atomicIncrementNumPxnsDone(1);
}

void CommandProcessor::waitForCommandProcessorsDone() {
  while (DrvAPI::getNumPxnsDone() != Drvx::getNodeDims().id) {
    hartYield();
  }
}

void CommandProcessor::finalize() {
  CommandProcessor::signalCommandProcessorDone();

  // wait for all CP to finish
  CommandProcessor::waitForCommandProcessorsDone();

  // set termination flag
  setTerminateFlag();

  // wait for all cores to complete finalization
  CommandProcessor::waitForCoresDone();
  SPDLOG_INFO("CP finalized on PXN {}", Drvx::getCurrentNode());
}

} // namespace pando
