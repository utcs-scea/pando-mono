// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "common.h"
#include "task.h"
#include <DrvAPI.hpp>
#include <cstdint>
#include <deque>
#include <inttypes.h>
#include <string>

using namespace DrvAPI;

/* shorthand for l1sp */
template <typename T>
using StaticL1SP = DrvAPIGlobalL1SP<T>;

/* shorthand for l2sp */
template <typename T>
using StaticL2SP = DrvAPIGlobalL2SP<T>;

/* shorthand for main-mem */
template <typename T>
using StaticMainMem = DrvAPIGlobalDRAM<T>;

/* a core's task queue */
struct task_queue {
  std::deque<task*> deque;
  std::mutex mtx;
  void push(task* t) {
    std::lock_guard<std::mutex> lock(mtx);
    deque.push_back(t);
  }
  task* pop() {
    std::lock_guard<std::mutex> lock(mtx);
    if (deque.empty()) {
      return nullptr;
    }
    task* t = deque.front();
    deque.pop_front();
    return t;
  }
};

static constexpr int64_t QUEUE_UNINIT = 0;
static constexpr int64_t QUEUE_INIT_IN_PROGRESS = 1;
static constexpr int64_t QUEUE_INIT = 2;

/* allocate one of these pointers on every core's l1 scratchpad */
StaticL1SP<int64_t> queue_initialized;         //!< set if initialized
StaticL1SP<task_queue*> this_cores_task_queue; //!< pointer to this core's task queue

/* allocate on of these per pxn */
StaticMainMem<int64_t> this_pxns_terminate;       //!< time for the thread to quit
StaticMainMem<int64_t> this_pxns_num_cores_ready; //!< number of cores ready to run

static DrvAPIPointer<int64_t> terminate_pointer() {
  /* pxn 0 holds the absolute */
  DrvAPIVAddress vaddr = static_cast<DrvAPIAddress>(&this_pxns_terminate);
  vaddr.not_scratchpad() = true;
  vaddr.pxn() = 0;
  return vaddr.encode();
}

static DrvAPIPointer<int64_t> num_cores_ready_pointer() {
  /* pxn 0 holds the absolute */
  DrvAPIVAddress vaddr = static_cast<DrvAPIAddress>(&this_pxns_num_cores_ready);
  vaddr.not_scratchpad() = true;
  vaddr.pxn() = 0;
  return vaddr.encode();
}

static int64_t num_cores() {
  return DrvAPI::numPXNs() * DrvAPI::numPXNPods() * DrvAPI::numPodCores();
}

/**
 * execute this task on a specific core
 */
void execute_on(uint32_t pxn, uint32_t pod, uint32_t core, task* t) {
  DrvAPIVAddress queue_vaddr = static_cast<DrvAPIAddress>(&this_cores_task_queue);
  queue_vaddr.global() = true;
  queue_vaddr.l2_not_l1() = false;
  queue_vaddr.pxn() = pxn;
  queue_vaddr.pod() = pod;
  queue_vaddr.core_y() = coreYFromId(core);
  queue_vaddr.core_x() = coreXFromId(core);
  DrvAPIPointer<task_queue*> queue_absolute_addr = queue_vaddr.encode();
  task_queue* tq = *queue_absolute_addr;
  tq->push(t);
}

/* every thread on every core in the system will call this function */
int Start(int argc, char* argv[]) {
  DrvAPIMemoryAllocatorInit();

  if (isCommandProcessor()) {
    // wait for all cores to be ready
    while (*num_cores_ready_pointer() != num_cores()) {
      nop(1000);
    }
    // only the command processor will run the main function
    pandoMain(argc, argv);
    atomic_add(terminate_pointer(), 1);
    return 0;
  }

  // check initialized
  if (atomic_cas(&queue_initialized, QUEUE_UNINIT, QUEUE_INIT_IN_PROGRESS) == QUEUE_UNINIT) {
    // initialize
    this_cores_task_queue = new task_queue;

    // indicate that initialization is complete
    queue_initialized = QUEUE_INIT;

    // only one thread on each core will reach this line of code
    atomic_add(num_cores_ready_pointer(), 1);
  }

  while (queue_initialized != QUEUE_INIT) {
    // wait for initialization to complete
    nop(1000);
  }

  while (*terminate_pointer() != numPXNs()) {
    task_queue* tq = this_cores_task_queue;
    task* t = tq->pop();
    if (t == nullptr) {
      nop(1000);
      continue;
    }
    t->execute();
    delete t;
  }
  return 0;
}

declare_drv_api_main(Start);
