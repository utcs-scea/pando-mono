// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "init.hpp"

#include "memory_resources.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/stdlib.hpp"
#include "specific_storage.hpp"
#include "start.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/config.hpp"
#include "prep/cores.hpp"
#include "prep/index.hpp"
#include "prep/log.hpp"
#include "prep/memory.hpp"
#include "prep/nodes.hpp"
#include "prep/status.hpp"
#ifdef PANDO_RT_ENABLE_MEM_STAT
#include "prep/memtrace_stat.hpp"
#endif
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cores.hpp"
#include "drvx/cp.hpp"
#include "drvx/log.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

void initialize() {
  initMemoryResources();

  // initialize task queues
#if defined(PANDO_RT_USE_BACKEND_PREP)

  if (isOnCP()) { // pandoMain thread
    // wait for all nodes to reach this point before invoking pandoMain to wait qthreads
    // initialization
    // this is called from the CP so no yield is necessary
    Nodes::barrier();
  }

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  if (isOnCP()) { // pandoMain thread
    if (auto status = CommandProcessor::initialize(); status != Status::Success) {
      PANDO_ABORT("CP was not initialized");
    }
  } else {
    Cores::initializeQueues();
  }

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

void finalize() {
  // finalize task queues
#ifdef PANDO_RT_USE_BACKEND_DRVX
  if (isOnCP()) { // pandoMain thread
    CommandProcessor::finalize();
  } else {
    Cores::finalizeQueues();
  }
#endif // PANDO_RT_USE_BACKEND_DRVX

  finalizeMemoryResources();
}

#ifdef PANDO_RT_USE_BACKEND_PREP

namespace {

// Powers on (boots up) the PANDO system.
// Equivalent to booting up the machine
[[nodiscard]] Status powerOn(int argc, char* argv[]) {
  if (auto status = Config::initialize(); status != Status::Success) {
    return status;
  }

  if (auto status = Nodes::initialize(); status != Status::Success) {
    return status;
  }

#ifdef PANDO_RT_ENABLE_MEM_STAT
  if (auto status = MemTraceStat::initialize(Nodes::getCurrentNode(), Nodes::getNodeDims());
      status != Status::Success) {
    return status;
  }
#endif

  // initializes memory and zeroes the first bytes that are required for global variables
  if (auto status = Memory::initialize(getReservedMemorySpace(MemoryType::L2SP),
                                       getReservedMemorySpace(MemoryType::Main));
      status != Status::Success) {
    return status;
  }

  // Initialize the cores/harts after nodes and memory are initialized
  if (auto status = Cores::initialize(&__start, argc, argv); status != Status::Success) {
    return status;
  }

  return Status::Success;
}

// Powers off (shuts down) the PANDO system.
// Equivalent to shutting down the machine.
void powerOff() {
  Cores::finalize();
  Memory::finalize();
  Nodes::finalize();

#ifdef PANDO_RT_ENABLE_MEM_STAT
  MemTraceStat::finalize();
#endif
}

// Returns the result of the application that was run on the CP.
int result() {
  return Cores::result();
}

} // namespace

#endif // PANDO_RT_USE_BACKEND_PREP

} // namespace pando

// Each backend is expected to have its own entry point that is responsible to spin up the CP/PH
// threads/harts, do backend specific initialization and then transfer control to __start() which
// will kickstart the user application.

#if defined(PANDO_RT_USE_BACKEND_PREP)

namespace pando {

// IIFE to initialize logger
const bool initLogger = [] {
  if (auto status = Logger::initialize(); status != Status::Success) {
    PANDO_ABORT("Logger could not be initialized");
    std::exit(static_cast<int>(status));
  }
  return true;
}();

} // namespace pando

// PREP entry point
int main(int argc, char* argv[]) {
  // initialize machine state (e.g., number of harts/cores/PXNs and memory sizes etc)
  if (auto status = pando::powerOn(argc, argv); status != pando::Status::Success) {
    PANDO_ABORT("PREP initialization failed");
  }

  int result = pando::result();

  pando::powerOff();

  return result;
}

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

namespace pando {

// IIFE to initialize logger
const bool initLogger = [] {
  if (auto status = Logger::initialize(); status != Status::Success) {
    PANDO_ABORT("Logger could not be initialized");
    std::exit(static_cast<int>(status));
  }
  return true;
}();

} // namespace pando

// drvx entry point
extern "C" __attribute__((visibility("default"))) int __drv_api_main(int argc, char* argv[]) {
  // DrvX assumes the first two args to be full paths to the application.so -- one for the CP to
  // load and the other for the PH cores to load. So, we adjust for the extra argument before
  // invoking start.
  const auto drvLibArgCount = 1;
  return __start(argc - drvLibArgCount, argv + drvLibArgCount);
}

#endif // PANDO_RT_USE_BACKEND_PREP