// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>

using namespace DrvAPI;

DrvAPI::DrvAPIGlobalL1SP<long> l1_done;
DrvAPI::DrvAPIGlobalL2SP<long> l2_done;

int FenceMain(int argc, char* argv[]) {
  if (myThreadId() == 0) {
    l2_done = 1;
    // this ensures that l2_done is visible to thread 1
    // before l1_done is visible to thread 1
    DrvAPI::fence();
    l1_done = 1;
  } else if (myThreadId() == 1) {
    while (true) {
      long l1 = l1_done;
      long l2 = l2_done;
      if (l1 && !l2) {
        printf("FAIL: l1_done is visible to thread 1 before l2_done\n");
        return 1;
      } else if (l2 && !l1) {
        printf("PASS 1/2: l2_done is visible to thread 1 before l1_done\n");
      } else if (l1 && l2) {
        printf("PASS 2/2: l1_done and l2_done are both visible to thread 1\n");
        return 0;
      }
    }
  }
  return 0;
}

declare_drv_api_main(FenceMain);
