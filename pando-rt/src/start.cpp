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

constexpr std::uint64_t STEAL_THRESH_HOLD_SIZE = 16;

constexpr bool IDLE_TIMER_ENABLE = false;
counter::Record<std::int64_t> idleCount = counter::Record<std::int64_t>();

enum SchedulerFailState{
  YIELD,
  STEAL,
};

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

    // PH hart
    if (thisPlace.core.x < coreDims.x) {
      // worker
      // executes tasks from the core's queue

      std::optional<pando::Task> task = std::nullopt;

      SchedulerFailState failState = SchedulerFailState::YIELD;

      do {
        idleTimer.start();
        task = queue->tryDequeue(ctok);

        if (!task.has_value()) {

          switch(failState) {
            case SchedulerFailState::YIELD:
#ifdef PANDO_RT_USE_BACKEND_PREP
              counter::recordHighResolutionEvent(idleCount, idleTimer, false, thisPlace.core.x, coreDims.x);
              pando::hartYield();
              //In Drvx hart yielding is a 1000 cycle wait which is too much
              idleTimer.start();
#endif
              failState = SchedulerFailState::STEAL;
              break;

            case SchedulerFailState::STEAL:
              for(std::int8_t i = 0; i < coreDims.x && !task.has_value(); i++) {
                auto* otherQueue =  pando::Cores::getTaskQueue(pando::Place{thisPlace.node, thisPlace.pod, pando::CoreIndex(i, 0)});
                if(!otherQueue || otherQueue == queue) {continue;}
                if(otherQueue->getApproxSize() > STEAL_THRESH_HOLD_SIZE) {
                  task = otherQueue->tryDequeue();
                }
              }
              failState = SchedulerFailState::YIELD;
              break;
          }
        }
        if(task.has_value()) {
          (*task)();
          task = std::nullopt;
        } else {
          counter::recordHighResolutionEvent(idleCount, idleTimer, false, thisPlace.core.x, coreDims.x);
        }
      } while (*coreActive == true);
    } else if (thisPlace.core.x == coreDims.x) {
      // scheduler
      // distributes tasks from the core's queue to other cores

      // initialize RNG with a static seed to ensure repeatability
      // we may want to introduce random seeds in future scheduler algorithms
      std::minstd_rand rng(thisPlace.core.x);
      // uniform distribution of potential target cores not including this scheduler core
      std::uniform_int_distribution<std::int8_t> dist(0, coreDims.x - 1);

      std::vector<pando::Queue<pando::Task>::ProducerToken> ptoks;
      for(std::uint8_t i = 0; i < coreDims.x; i++){
        const pando::Place dstPlace(thisPlace.node, thisPlace.pod, pando::CoreIndex(i, 0));
        auto* otherQueue =  pando::Cores::getTaskQueue(dstPlace);
        ptoks.push_back(otherQueue->makeProducerToken());
      }


      do {
        // distribute tasks from the queue
        auto task = queue->tryDequeue(ctok);

        if (task.has_value()) {
          // enqueue in a random core in this node and pod
          auto coreIdx = dist(rng);
          const pando::Place dstPlace(thisPlace.node, thisPlace.pod,
                                      pando::CoreIndex(coreIdx, 0));
          auto* dstQueue = pando::Cores::getTaskQueue(dstPlace);
          if (auto status = dstQueue->enqueue(ptoks[coreIdx], std::move(task.value()));
              status != pando::Status::Success) {
            PANDO_ABORT("Could not enqueue from scheduler to worker core");
          }
        }
      } while (*coreActive == true);
    }
  }

  pando::finalize();

  return result;
}
