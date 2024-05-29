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
Drvx::StaticL1SP<std::underlying_type_t<CoreState>> coreState;

// Per-PXN (main memory) variables
Drvx::StaticMainMem<std::int64_t> numPxnsDone;
Drvx::StaticMainMem<std::int64_t> numCoresReady;

} // namespace

void Cores::initializeQueues() {
  // One hart per core does all the initialization. Use CAS to choose some/any hart for this
  // purpose.
#if defined(PANDO_RT_BYPASS_INIT)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(&coreState, &addr_native, &size);
  std::int8_t* as_native_pointer = reinterpret_cast<std::int8_t*>(addr_native);
  std::int8_t expected = +CoreState::Stopped;
  std::int8_t desired = +CoreState::Idle;
  __atomic_compare_exchange_n(as_native_pointer, &expected, desired, false, static_cast<int>(std::memory_order_relaxed), static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);

  if (expected == +CoreState::Stopped) {
#else
  if (DrvAPI::atomic_cas(&coreState, +CoreState::Stopped, +CoreState::Idle) == +CoreState::Stopped) {
#endif
    // initialize core object
    coreQueue = new TaskQueue;

    // indicate that core initialization is complete and state is ready
    coreState = +CoreState::Ready;

    // CP waits for this variable to equal total number of cores in the PXN
#if defined(PANDO_RT_BYPASS_INIT)
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(numCoresReady, NodeIndex(0)), &addr_native, &size);
    std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
    __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
#else
    DrvAPI::atomic_add(toNativeDrvPointerOnDram(numCoresReady, NodeIndex(0)), 1u);
#endif
  }

  // Other harts just wait until state is ready
  while (coreState != +CoreState::Ready) {
    hartYield();
  }
}

void Cores::finalizeQueues() {
  // One hart per core does all the finalization. Use CAS to choose some/any hart for this
  // purpose.
#if defined(PANDO_RT_BYPASS_INIT)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(&coreState, &addr_native, &size);
  std::int8_t* as_native_pointer = reinterpret_cast<std::int8_t*>(addr_native);
  std::int8_t expected = +CoreState::Ready;
  std::int8_t desired = +CoreState::Idle;
  __atomic_compare_exchange_n(as_native_pointer, &expected, desired, false, static_cast<int>(std::memory_order_relaxed), static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);

  if (expected == +CoreState::Ready) {
#else
  if (DrvAPI::atomic_cas(&coreState, +CoreState::Ready, +CoreState::Idle) == +CoreState::Ready) {
#endif
    coreState = +CoreState::Stopped;

    TaskQueue* queue = coreQueue;
    delete queue;

    CommandProcessor::signalCoresDone();
  }

  // Other harts simply exit
}

bool Cores::CoreActiveFlag::operator*() const noexcept {
  hartYield();
  return (*toNativeDrvPointerOnDram(numPxnsDone, NodeIndex(0)) != Drvx::getNodeDims().id);
}

Cores::TaskQueue* Cores::getTaskQueue(Place place) noexcept {
  if(*toNativeDrvPtr(coreState, place) != +CoreState::Ready) return nullptr;
  return *toNativeDrvPtr(coreQueue, place);
}

Cores::CoreActiveFlag Cores::getCoreActiveFlag() noexcept {
  return CoreActiveFlag{};
}

void Cores::waitForCoresInit() {
  while (*toNativeDrvPointerOnDram(numCoresReady, NodeIndex(0)) != Drvx::getNumSystemCores()) {
    hartYield();
  }
}

void Cores::finalize() {
#if defined(PANDO_RT_BYPASS_INIT)
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

} // namespace pando
