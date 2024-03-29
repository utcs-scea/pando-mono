// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "cores.hpp"

#include <stdlib.h> // required for setenv

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "qthread/barrier.h"
#include "qthread/qthread.h"

#include "config.hpp"
#include "hart_context.hpp"
#include "index.hpp"
#include "log.hpp"
#include "status.hpp"

namespace pando {

namespace {

// Args of user function
struct Args {
  std::int32_t m_argc{};
  char** m_argv{};
  int m_result{};

  Status init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    return Status::Success;
  }

  std::int32_t argc() const noexcept {
    return m_argc;
  }

  char** argv() const noexcept {
    return m_argv;
  }

  void setResult(int result) {
    m_result = result;
  }

  int result() const noexcept {
    return m_result;
  }
} args;

} // namespace

// An emulated PANDOHammer core.
//
// Objects of this class are used to emulate a PANDOHammer core. That includes everything necessary
// for enqueuing and scheduling work and any associated metadata and metrics.
class ComputeCore {
public:
  using IDType = std::int32_t;

private:
  // Emulated hart loop. This function has the common functionality to setup a hart for processing.
  static aligned_t hartLoop(void* arg) noexcept {
    auto* thisContext = static_cast<HartContext*>(arg);

    SPDLOG_INFO("Core {}:{} started (context={}, shepherd={})", thisContext->core->id(),
                thisContext->id, static_cast<void*>(thisContext), qthread_shep());

    // We assume that stack grows from higher addresses to lower addresses. thisContext is the guard
    // that has the start of the stack, which is the highest address, i.e., the end of the range and
    // (&thisContext - qthread_stackleft()) gives the start of the range (lowest address).
    //
    // qthread_stackleft() is inaccurate (~10bytes according to qthreads manpages). If it's over by
    // 10bytes a GlobalPtr could potentially be created on space not allowed, but it does not matter
    // as GlobalPtr should not be created outside this function.
    //
    // If it's under by 10bytes, then one of the variables in the function may end up not being
    // translated properly. This will be caught as an error as translation will fail. It may not
    // matter, as it is difficult to create and use a GlobalPtr within 10bytes due to the additional
    // pressure due to the communication stack.
    thisContext->stackAddressRange = {
        reinterpret_cast<std::byte*>(&thisContext) - qthread_stackleft(),
        reinterpret_cast<std::byte*>(&thisContext)};

    // wait for all qthreads to set their stack information
    qt_barrier_enter(thisContext->core->m_hartBarrier);

    // set hart context
    if (auto status = hartContextSet(thisContext); status != Status::Success) {
      SPDLOG_ERROR("Core {}:{} failed to set context: {}", thisContext->core->id(), thisContext->id,
                   status);
      std::abort();
    }

    // Call the entry point for all PH harts, which is also separately called by the CP.
    int result = (*thisContext->entry)(args.argc(), args.argv());

    // reset hart context and exit
    hartContextReset();

    SPDLOG_INFO("Core {}:{} stopped (context={}, shepherd={})", thisContext->core->id(),
                thisContext->id, static_cast<void*>(thisContext), qthread_shep());

    return result;
  }

  IDType m_id{};
  std::atomic<bool> m_active{false};
  Cores::TaskQueue m_taskQueue;
  std::vector<HartContext> m_hartContexts;
  qt_barrier_t* m_hartBarrier{};

public:
  // Starts all threads associated with this core
  [[nodiscard]] Status start(IDType coreId, std::size_t hartCount, int (*entry)(int, char*[])) {
    if (m_active.load(std::memory_order_relaxed) != false) {
      return Status::AlreadyInit;
    }

    // initialize core
    m_id = coreId;

    // mark the core as active
    m_active.store(true, std::memory_order_relaxed);

    // initialize qthread barrier and include this thread
    if (m_hartBarrier = qt_barrier_create(hartCount + 1, qt_barrier_btype::REGION_BARRIER);
        m_hartBarrier == nullptr) {
      return Status::InitError;
    }

    // start qthreads
    m_hartContexts.reserve(hartCount);
    const qthread_shepherd_id_t shepherd = m_id + 1;
    for (std::size_t hartID = 0; hartID < hartCount; ++hartID) {
      m_hartContexts.emplace_back(ThreadIndex(hartID), this, entry);
      auto& hartContext = m_hartContexts[hartID];
      auto& result = hartContext.result;

      // empty result
      if (auto status = qthread_empty(&result); status != QTHREAD_SUCCESS) {
        return Status::InitError;
      }

      // create qthread
      if (auto status = qthread_fork_to(&hartLoop, &hartContext, &result, shepherd);
          status != QTHREAD_SUCCESS) {
        return Status::LaunchError;
      }
    }

    // wait for all qthreads to set their stack information
    qt_barrier_enter(m_hartBarrier);

    return Status::Success;
  }

  // Stops all threads associated with this core and waits for them to join
  Status stop() noexcept {
    if (!m_active.load(std::memory_order_relaxed)) {
      return Status::NotInit;
    }

    // flag qthreads to stop
    m_active.store(false, std::memory_order_relaxed);

    // wait for each qthread to stop
    for (std::size_t hartId = 0; hartId < m_hartContexts.size(); ++hartId) {
      auto& hartContext = m_hartContexts[hartId];
      auto& result = hartContext.result;
      if (auto status = qthread_readFF(nullptr, &result); status != QTHREAD_SUCCESS) {
        return Status::MemoryError;
      }
    }

    m_hartContexts.clear();
    m_taskQueue.clear();

    qt_barrier_destroy(m_hartBarrier);
    m_hartBarrier = nullptr;

    return Status::Success;
  }

  CoreIndex id() const noexcept {
    return CoreIndex(m_id, 0);
  }

  // Returns the task queue associated with this core
  Cores::TaskQueue* getQueue() noexcept {
    return &m_taskQueue;
  }

  // Returns if the core is active
  bool isActive() const noexcept {
    return m_active.load(std::memory_order_relaxed);
  }

  // Returns the L1SP offset for the given hart ID and offset in its stack
  std::size_t getL1SPOffset(HartContext::IDType hartId,
                            std::size_t hartStackOffset) const noexcept {
    auto&& l1SPsize = Config::getCurrentConfig().memory.l1SPHart;
    return hartId.id * l1SPsize + hartStackOffset;
  }

  // Returns the native address for an offset off L1SP
  void* getNativeAddress(std::size_t l1SPOffset) const noexcept {
    auto&& l1SPsize = Config::getCurrentConfig().memory.l1SPHart;
    // find the hart ID and the offset from the start of its stack
    const auto hartId = l1SPOffset / l1SPsize;
    const std::size_t hartStackOffset = l1SPOffset - (hartId * l1SPsize);
    return m_hartContexts[hartId].getStackAddress(hartStackOffset);
  }
};

namespace {

class ComputeNode {
  std::vector<ComputeCore> m_cores;

public:
  // Starts the compute node and all its cores
  [[nodiscard]] Status start(std::size_t coreCount, std::size_t hartsPerCore,
                             int (*entry)(int, char*[])) {
    // start cores
    m_cores = std::vector<ComputeCore>(coreCount);
    for (ComputeCore::IDType coreId = 0; coreId < static_cast<ComputeCore::IDType>(m_cores.size());
         ++coreId) {
      if (auto status = m_cores[coreId].start(coreId, hartsPerCore, entry);
          status != Status::Success) {
        SPDLOG_ERROR("Error initializing core {}: {}", coreId, status);
        return status;
      }
    }

    // run CP code and set the return code
    args.setResult((*entry)(args.argc(), args.argv()));

    return Status::Success;
  }

  // Stops all cores associated with this node and waits for them to join
  Status stop() noexcept {
    // stop cores backwards to stop the scheduler core first
    std::for_each(m_cores.rbegin(), m_cores.rend(), [](auto& core) {
      core.stop();
    });
    m_cores.clear();

    return Status::Success;
  }

  // Returns the cores of this node
  std::vector<ComputeCore>& getCores() noexcept {
    return m_cores;
  }

  ComputeCore& getCore(CoreIndex coreIdx) noexcept {
    return m_cores[coreIdx.x];
  }
};

// Singleton node object
ComputeNode node;

} // namespace

bool Cores::CoreActiveFlag::operator*() const noexcept {
  auto& hartContext = *static_cast<HartContext*>(internalData);
  hartYield(hartContext);
  return hartContext.core->isActive();
}

Status Cores::initialize(int (*entry)(int, char**), int argc, char* argv[]) {
  const auto& config = Config::getCurrentConfig();

  {
    // set stack size
    auto stackSize = fmt::format("{}", config.memory.l1SPHart);
    setenv("QTHREAD_STACK_SIZE", stackSize.c_str(), 0);
  }

  {
    // set number of shepherds; needs to be +1 as the main thread is counted as one
    auto shepherdCount = fmt::format("{}", config.compute.coreCount + 1);
    setenv("QTHREAD_NUM_SHEPHERDS", shepherdCount.c_str(), 0);
  }

  // initialize qthread library
  if (auto status = qthread_initialize(); status != 0) {
    SPDLOG_ERROR("Error initializing qthreads: {}", status);
    return Status::InitError;
  }

  // check if task local is enough for hart contexts
  if (auto taskLocalSize = qthread_size_tasklocal(); taskLocalSize < sizeof(HartContext*)) {
    SPDLOG_ERROR(
        "Insufficient space for hart contexts: {} bytes required but only {} bytes available",
        sizeof(HartContext*), taskLocalSize);
    return Status::InsufficientSpace;
  }

  SPDLOG_INFO("Cores initialized with qthreads: shepherds={}, workers={}, stack={}",
              qthread_readstate(ACTIVE_SHEPHERDS), qthread_readstate(ACTIVE_WORKERS),
              qthread_readstate(STACK_SIZE));

  SPDLOG_INFO("CP started (shepherd={})", qthread_shep());

  // set program args
  if (auto status = args.init(argc, argv); status != Status::Success) {
    SPDLOG_ERROR("Error initializing program args: {}", status);
    return status;
  }

  // start all cores in the compute node
  if (auto status = node.start(config.compute.coreCount, config.compute.hartCount, entry);
      status != Status::Success) {
    SPDLOG_ERROR("Error initializing node: {}", status);
    return status;
  }

  return Status::Success;
}

void Cores::finalize() {
  // stop all compute cores in the compute node
  node.stop();
  SPDLOG_INFO("CP stopped (shepherd={})", qthread_shep());
  qthread_finalize();
}

PodIndex Cores::getCurrentPod() noexcept {
  auto currentContext = hartContextGet();
  if (currentContext == nullptr) {
    // the CP does not have pods
    return anyPod;
  }
  return PodIndex{0, 0};
}

CoreIndex Cores::getCurrentCore() noexcept {
  auto currentContext = hartContextGet();
  if (currentContext == nullptr) {
    // the CP does not have cores
    return anyCore;
  }
  return currentContext->core->id();
}

std::tuple<PodIndex, CoreIndex> Cores::getCurrentPodAndCore() noexcept {
  auto currentContext = hartContextGet();
  if (currentContext == nullptr) {
    // the CP does not have pods or cores
    return std::make_tuple(anyPod, anyCore);
  }
  return std::make_tuple(PodIndex{0, 0}, currentContext->core->id());
}

PodIndex Cores::getPodDims() noexcept {
  return PodIndex{1, 1};
}

CoreIndex Cores::getCoreDims() noexcept {
  return CoreIndex{static_cast<std::int8_t>(node.getCores().size()), 1};
}

ThreadIndex Cores::getCurrentThread() noexcept {
  auto currentContext = hartContextGet();
  if (currentContext == nullptr) {
    // the CP does not set have harts
    return ThreadIndex{-1};
  }
  return currentContext->id;
}

ThreadIndex Cores::getThreadDims() noexcept {
  return ThreadIndex{static_cast<std::int8_t>(Config::getCurrentConfig().compute.hartCount)};
}

std::ptrdiff_t Cores::getL1SPOffset(const void* p) noexcept {
  auto currentContext = hartContextGet();
  if (currentContext == nullptr) {
    // CP does not support stack global addressing
    return -1;
  }

  const auto hartStackOffset = currentContext->getStackOffset(p);
  if (hartStackOffset < 0) {
    return hartStackOffset;
  }
  return currentContext->core->getL1SPOffset(currentContext->id, hartStackOffset);
}

void* Cores::getL1SPLocalAdddress(PodIndex podIdx, CoreIndex coreIdx, std::size_t offset) noexcept {
  const auto& podDims = getPodDims();
  if ((podIdx.x < 0 || podIdx.y < 0) || (podIdx.x >= podDims.x || podIdx.y >= podDims.y)) {
    SPDLOG_ERROR("Invalid pod index: {}", podIdx);
    return nullptr;
  }

  const auto& coreDims = getCoreDims();
  if ((coreIdx.x < 0 || coreIdx.y < 0) || (coreIdx.x >= coreDims.x || coreIdx.y >= coreDims.y)) {
    SPDLOG_ERROR("Invalid core index: {}", coreIdx);
    return nullptr;
  }

  const auto& core = node.getCore(coreIdx);
  return core.getNativeAddress(offset);
}

int Cores::result() noexcept {
  return args.result();
}

Cores::TaskQueue* Cores::getTaskQueue(Place place) noexcept {
  return node.getCore(place.core).getQueue();
}

Cores::CoreActiveFlag Cores::getCoreActiveFlag() noexcept {
  return CoreActiveFlag{hartContextGet()};
}

} // namespace pando
