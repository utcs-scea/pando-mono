// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>
#include <string>
using namespace DrvAPI;

DrvAPIGlobalL2SP<int64_t> lock;

int CPMain(int argc, char* argv[]) {
  DrvAPIMemoryAllocatorInit();
  if (!isCommandProcessor())
    return 0;

  DrvAPIPointer<void> p = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, 1024);

  printf("PXN %2d: Pod %2d: Core %2d: Thread %2d: p = %" PRIx64 "(%s)\n", myPXNId(), myPodId(),
         myCoreId(), myThreadId(), static_cast<DrvAPIAddress>(p),
         DrvAPIVAddress(p).to_string().c_str());

  return 0;
}

declare_drv_api_main(CPMain);
