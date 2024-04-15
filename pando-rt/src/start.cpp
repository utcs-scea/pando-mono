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

#ifdef PANDO_RT_USE_BACKEND_PREP
#include "prep/cores.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cores.hpp"
#endif

constexpr std::uint64_t CHUNK_SIZE = 8192;

extern "C" int __start(int argc, char** argv) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto coreDims = pando::getCoreDims();
  int result = 0;

  pando::initialize();

  if (pando::isOnCP()) {
    // invokes user's main function (pandoMain)
    result = pandoMain(argc, argv);
  } else {
    auto* queue = pando::Cores::getTaskQueue(thisPlace);
    auto coreActive = pando::Cores::getCoreActiveFlag();

    // PH hart
    if (thisPlace.core.x < coreDims.x) {
      // worker
      // executes tasks from the core's queue

      std::optional<pando::Task> task = std::nullopt;

      do {
        if (!task.has_value()) {
          task = queue->tryDequeue();
        } else {
          for(std::int8_t i = 0; i <= coreDims.x && !task.has_value(); i++) {
            auto* otherQueue =  pando::Cores::getTaskQueue(pando::Place{thisPlace.node, thisPlace.pod, pando::CoreIndex(i, 0)});
            if(otherQueue == queue) {continue;}
            if(otherQueue->getApproxSize() > CHUNK_SIZE) {
              task = otherQueue->tryDequeue();
            }
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

      do {
        // distribute tasks from the queue
        if (auto task = queue->tryDequeue(); task.has_value()) {
          // enqueue in a random core in this node and pod
          const pando::Place dstPlace(thisPlace.node, thisPlace.pod,
                                      pando::CoreIndex(dist(rng), 0));
          auto* dstQueue = pando::Cores::getTaskQueue(dstPlace);
          if (auto status = dstQueue->enqueue(std::move(task.value()));
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
