// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "start.hpp"

#include <sys/resource.h>
#include <sys/time.h>
#include <chrono>
#include <random>
#include <utility>

#include "init.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/main.hpp"
#include "pando-rt/status.hpp"
#include "pando-rt/stdlib.hpp"
#include "pando-rt/pando-rt.hpp"
#include "spdlog/spdlog.h"

#ifdef PANDO_RT_USE_BACKEND_PREP
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cores.hpp"
#include "drvx/drvx.hpp"
#endif

constexpr std::uint64_t STEAL_THRESH_HOLD_SIZE = 4096;
std::array<std::int64_t,66> counts;

enum SchedulerFailState{
  YIELD,
  STEAL,
};

extern "C" int __start(int argc, char** argv) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto coreDims = pando::getCoreDims();
  int result = 0;

  pando::initialize();


  struct rusage start, end;

  if (pando::isOnCP()) {
    for(std::uint64_t i = 0; i < counts.size(); i++) {
      counts[i] = 0;
    }
    auto rc = getrusage(RUSAGE_SELF, &start);
    if(rc != 0) {PANDO_ABORT("GETRUSAGE FAILED");}
    // invokes user's main function (pandoMain)
    result = pandoMain(argc, argv);
    rc = getrusage(RUSAGE_SELF, &end);
    if(rc != 0) {PANDO_ABORT("GETRUSAGE FAILED");}
  } else {
    auto* queue = pando::Cores::getTaskQueue(thisPlace);
    auto coreActive = pando::Cores::getCoreActiveFlag();

    auto ctok = queue->makeConsumerToken();

    // PH hart
    if (thisPlace.core.x < coreDims.x) {
      // worker
      // executes tasks from the core's queue

      std::optional<pando::Task> task = std::nullopt;

      SchedulerFailState failState = SchedulerFailState::YIELD;

      do {
        task = queue->tryDequeue(ctok);
        if (!task.has_value()) {
          switch(failState) {
            case SchedulerFailState::YIELD:
#ifdef PANDO_RT_USE_BACKEND_PREP
              pando::hartYield();
              //In Drvx hart yielding is a 1000 cycle wait which is too much
#endif
              failState = SchedulerFailState::STEAL;
              break;
            case SchedulerFailState::STEAL:
              for(std::int8_t i = 0; i <= coreDims.x && !task.has_value(); i++) {
                auto* otherQueue =  pando::Cores::getTaskQueue(pando::Place{thisPlace.node, thisPlace.pod, pando::CoreIndex(i, 0)});
                if(otherQueue == queue) {continue;}
                if(otherQueue->getApproxSize() > STEAL_THRESH_HOLD_SIZE) {
                  task = otherQueue->tryDequeue();
                }
              }
              failState = SchedulerFailState::YIELD;
              break;
          }
        }
        if(task.has_value()) { (*task)(); task = std::nullopt; }
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
        const pando::Place dstPlace(thisPlace.node, thisPlace.pod,
                                    pando::CoreIndex(i, 0));
        auto* otherQueue =  pando::Cores::getTaskQueue(pando::Place{thisPlace.node, thisPlace.pod, pando::CoreIndex(i, 0)});
        ptoks.push_back(otherQueue->makeProducerToken());
      }

      do {
        // distribute tasks from the queue
        if (auto task = queue->tryDequeue(ctok); task.has_value()) {
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

  if(pando::isOnCP()) {
    SPDLOG_CRITICAL("Total time on node: {}, was {}ns",
        thisPlace.node.id,
        end.ru_utime.tv_sec * 1000000000 + end.ru_utime.tv_usec * 1000 -
        (start.ru_utime.tv_sec * 1000000000 + start.ru_utime.tv_usec * 1000));
  }

  if(pando::isOnCP() || pando::getCurrentThread().id == 0) {
    SPDLOG_CRITICAL("Pointer time on node: {}, core: {} was {}",
        thisPlace.node.id,
        thisPlace.core.x,
        counts[pando::isOnCP() ? coreDims.x  + 1 : thisPlace.core.x]);
  }
  return result;
}
