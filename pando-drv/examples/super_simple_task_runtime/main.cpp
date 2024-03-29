// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "common.h"
#include "task.h"
#include <DrvAPI.hpp>
#include <inttypes.h>
#include <stdio.h>
#include <vector>

using namespace DrvAPI;

int pandoMain(int argc, char* argv[]) {
  // in this test just demonastrate that
  // pxn 0 can send pxn 1 a task

  pr_info("hello, from pandoMain running on a command processor\n");

  std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> places = {
      {myPXNId(), 0, 0},
      {myPXNId(), numPXNPods() - 1, 0},
      {myPXNId(), numPXNPods() - 1, numPodCores() - 1},
      {numPXNs() - 1, 0, 0},
      {numPXNs() - 1, numPXNPods() - 1, 0},
      {numPXNs() - 1, numPXNPods() - 1, numPodCores() - 1},
  };

  DrvAPIPointer<size_t> done = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(int));
  *done = 0;

  for (auto& place : places) {
    uint32_t src_pxn = myPXNId();
    uint32_t pxn, pod, core;
    std::tie(pxn, pod, core) = place;
    execute_on(pxn, pod, core, newTask([src_pxn, done]() {
                 pr_info("hello, from task sent by command processor on PXN %" PRIu32 "\n",
                         src_pxn);
                 atomic_add(done, 1);
                 return 0;
               }));
  }

  while (*done != places.size()) {
    DrvAPI::wait(1000);
  }

  return 0;
}
