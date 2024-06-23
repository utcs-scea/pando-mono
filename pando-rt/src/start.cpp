// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "start.hpp"

#include <random>
#include <utility>

#include "init.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/main.hpp"
#include "pando-rt/status.hpp"
#include "pando-rt/stdlib.hpp"
#include "pando-rt/pando-rt.hpp"
#include <pando-rt/benchmark/counters.hpp>

#ifdef PANDO_RT_USE_BACKEND_PREP
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cores.hpp"
#include "drvx/drvx.hpp"
#endif

counter::Record<std::int64_t> idleCount = counter::Record<std::int64_t>();

extern "C" int __start(int argc, char** argv) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto coreDims = pando::getCoreDims();
  int result = 0;

  pando::initialize();

  counter::HighResolutionCount<IDLE_TIMER_ENABLE> idleTimer;

  if (pando::isOnCP()) {
    // invokes user's main function (pandoMain)
    result = pandoMain(argc, argv);
  } else {
    auto* queue = pando::Cores::getTaskQueue(thisPlace);
    if(pando::getCurrentThread().id == 0){
      SPDLOG_WARN("Node: {}, core: {}, queue {}", thisPlace.node.id, thisPlace.core.x, (void*)queue);
    }

    auto coreActive = pando::Cores::getCoreActiveFlag();
    auto ctok = queue->makeConsumerToken();

    std::optional<pando::Task> task = std::nullopt;

#if defined(PANDO_RT_USE_BACKEND_PREP)
    SchedulerFailState failState = SchedulerFailState::YIELD;
#endif

    do {
      idleTimer.start();
      task = queue->tryDequeue(ctok);

      if (!task.has_value()) {
#ifdef PANDO_RT_USE_BACKEND_PREP
        pando::Cores::workStealing(task, failState, idleCount, idleTimer);
#elif defined(PANDO_RT_USE_BACKEND_DRVX) && defined(PANDO_RT_WORK_STEALING)
        pando::Cores::workStealing(task);
#endif
      }

      // After work stealing
      if(task.has_value()) {
        (*task)();
        task = std::nullopt;
      } else {
        counter::recordHighResolutionEvent(idleCount, idleTimer, false, thisPlace.core.x, coreDims.x);
      }
    } while (*coreActive == true);
  }

  pando::finalize();

  return result;
}
