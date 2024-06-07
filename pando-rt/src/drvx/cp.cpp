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

// Per-PXN (main memory) variables
Drvx::StaticMainMem<std::int64_t> globalBarrierCounter;
Drvx::StaticMainMem<std::int64_t> pxnBarrierDone;
Drvx::StaticMainMem<std::int64_t> coresDone;
Drvx::StaticMainMem<std::int64_t> numPxnsDone;

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
    *toNativeDrvPointerOnDram(pxnBarrierDone, Drvx::getCurrentNode()) = 0u;

    // enter the global barrier by incrementing the global counter on PXN-0
#if defined(PANDO_RT_BYPASS)
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(globalBarrierCounter, NodeIndex(0)), &addr_native, &size);
    std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
    std::int64_t curBarrierCount = __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
#else
    std::int64_t curBarrierCount = DrvAPI::atomic_add(toNativeDrvPointerOnDram(globalBarrierCounter, NodeIndex(0)), 1u);
#endif

    if (curBarrierCount == Drvx::getNodeDims().id - 1) {
      // last PXN to reach barrier; reset global barrier counter and signal to other PXNs that we
      // are done
      *toNativeDrvPointerOnDram(globalBarrierCounter, NodeIndex(0)) = 0u;
      for (std::int64_t i = 0; i < Drvx::getNodeDims().id; ++i) {
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
#if defined(PANDO_RT_BYPASS)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(coresDone, NodeIndex(pando::getCurrentNode().id)), &addr_native, &size);
  std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
  __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);
#else
  DrvAPI::atomic_add(toNativeDrvPointerOnDram(coresDone, NodeIndex(pando::getCurrentNode().id)), 1u);
#endif
}

void CommandProcessor::waitForCoresDone() {
  while (*toNativeDrvPointerOnDram(coresDone, NodeIndex(pando::getCurrentNode().id)) != Drvx::getCoreDims().x) {
    hartYield();
  }
}

void CommandProcessor::signalCommandProcessorDone() {
#if defined(PANDO_RT_BYPASS)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(numPxnsDone, NodeIndex(0)), &addr_native, &size);
  std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
  __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);
#else
  DrvAPI::atomic_add(toNativeDrvPointerOnDram(numPxnsDone, NodeIndex(0)), 1u);
#endif
}

void CommandProcessor::waitForCommandProcessorsDone() {
  while (*toNativeDrvPointerOnDram(numPxnsDone, NodeIndex(0)) != Drvx::getNodeDims().id) {
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
