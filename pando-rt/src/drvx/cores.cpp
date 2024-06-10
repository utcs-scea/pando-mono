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
#include "termination.hpp"

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
Drvx::StaticL1SP<std::int64_t> numHartsDone;

// Per-PXN (main memory) variables
Drvx::StaticMainMem<std::int64_t> numCoresInitialized;
Drvx::StaticMainMem<std::int64_t> numCoresFinalized;

} // namespace

void Cores::initializeQueues() {
  // One hart per core does all the initialization. Use CAS to choose some/any hart for this
  // purpose.
#if defined(PANDO_RT_BYPASS)
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
    DrvAPI::write(&coreState, +CoreState::Ready);

    // CP waits for this variable to equal total number of cores in the PXN
#if defined(PANDO_RT_BYPASS)
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(numCoresInitialized, NodeIndex(pando::getCurrentNode().id)), &addr_native, &size);
    std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
    __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
#else
    DrvAPI::atomic_add(toNativeDrvPointerOnDram(numCoresInitialized, NodeIndex(pando::getCurrentNode().id)), 1u);
#endif
  }

  // Other harts just wait until state is ready
  while (DrvAPI::read<std::int8_t>(&coreState) != +CoreState::Ready) {
    hartYield();
  }
}

void Cores::finalizeQueues() {
  // One hart per core does all the finalization. Use CAS to choose some/any hart for this
  // purpose.
  Cores::signalHartDone();

#if defined(PANDO_RT_BYPASS)
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
    // wait for all harts on this core to be done
    Cores::waitForHartsDone();

    Cores::signalCoreFinalized();

    // wait for all cores on this PXN to be finalized
    Cores::waitForCoresFinalized();

    DrvAPI::write(&coreState, +CoreState::Stopped);

    TaskQueue* queue = coreQueue;
    delete queue;

    CommandProcessor::signalCoresDone();
  }

  // Other harts simply exit
}

Cores::CoreActiveFlag Cores::getCoreActiveFlag() noexcept {
  return CoreActiveFlag{};
}

bool Cores::CoreActiveFlag::operator*() const noexcept {
  hartYield();
  return !getTerminateFlag();
}

Cores::TaskQueue* Cores::getTaskQueue(Place place) noexcept {
  return *toNativeDrvPtr(coreQueue, place);
}

void Cores::waitForCoresInitialized() {
  while (*toNativeDrvPointerOnDram(numCoresInitialized, NodeIndex(pando::getCurrentNode().id)) != Drvx::getCoreDims().x) {
    hartYield();
  }
}

void Cores::signalHartDone() {
#if defined(PANDO_RT_BYPASS)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(toNativeDrvPtr(numHartsDone, getCurrentPlace()), &addr_native, &size);
  std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
  __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);
#else
  DrvAPI::atomic_add(toNativeDrvPtr(numHartsDone, getCurrentPlace()), 1u);
#endif
}

void Cores::waitForHartsDone() {
  while (DrvAPI::read<std::int64_t>(toNativeDrvPtr(numHartsDone, getCurrentPlace())) != Drvx::getThreadDims().id) {
    hartYield();
  }
}

void Cores::signalCoreFinalized() {
#if defined(PANDO_RT_BYPASS)
  void *addr_native = nullptr;
  std::size_t size = 0;
  DrvAPI::DrvAPIAddressToNative(toNativeDrvPointerOnDram(numCoresFinalized, NodeIndex(pando::getCurrentNode().id)), &addr_native, &size);
  std::int64_t* as_native_pointer = reinterpret_cast<std::int64_t*>(addr_native);
  __atomic_fetch_add(as_native_pointer, 1u, static_cast<int>(std::memory_order_relaxed));
  // hartYield
  DrvAPI::nop(1u);
#else
  DrvAPI::atomic_add(toNativeDrvPointerOnDram(numCoresFinalized, NodeIndex(pando::getCurrentNode().id)), 1u);
#endif
}

void Cores::waitForCoresFinalized() {
  while (*toNativeDrvPointerOnDram(numCoresFinalized, NodeIndex(pando::getCurrentNode().id)) != Drvx::getCoreDims().x) {
    hartYield();
  }
}

void Cores::finalize() {
}

} // namespace pando
