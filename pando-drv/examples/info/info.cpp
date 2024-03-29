// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>

int SimpleMain(int argc, char* argv[]) {
  printf(
      "my pxn: %2d/%2d, "
      "my pod: %2d/%2d, "
      "my core: %2d/%2d, "
      "my thread: %2d/%2d \n",
      DrvAPI::myPXNId(), DrvAPI::numPXNs(), DrvAPI::myPodId(), DrvAPI::numPXNPods(),
      DrvAPI::myCoreId(), DrvAPI::numPodCores(), DrvAPI::myThreadId(), DrvAPI::numCoreThreads());
  if (DrvAPI::myPXNId() == 0 && DrvAPI::myPodId() == 0 && DrvAPI::myCoreId() == 0 &&
      DrvAPI::myThreadId() == 0) {
    printf("core l1sp size = %" PRIu64 " bytes\n", DrvAPI::coreL1SPSize());
    printf("pod l2sp size  = %" PRIu64 " bytes\n", DrvAPI::podL2SPSize());
    printf("pxn dram size  = %" PRIu64 " bytes\n", DrvAPI::pxnDRAMSize());
  }
  return 0;
}

declare_drv_api_main(SimpleMain);
