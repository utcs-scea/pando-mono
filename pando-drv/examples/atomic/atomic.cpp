// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>

#if !defined(TARGET_L1SP) && !defined(TARGET_L2SP) && !defined(TARGET_DRAM)
#define TARGET_DRAM
#endif

#if defined(TARGET_L1SP)
DrvAPI::DrvAPIGlobalL1SP<int64_t>
#elif defined(TARGET_L2SP)
DrvAPI::DrvAPIGlobalL2SP<int64_t>
#elif defined(TARGET_DRAM)
DrvAPI::DrvAPIGlobalDRAM<int64_t>
#endif
    target;

#ifdef MEMOP_ADD
#define memop(addr) DrvAPI::atomic_add<int64_t>(addr, 1)
#elif defined(MEMOP_LOAD)
#define memop(addr) DrvAPI::read<int64_t>(addr)
#elif defined(MEMOP_STORE)
#define memop(addr) DrvAPI::write<int64_t>(addr, 1)
#endif

#ifndef NATOMICS
#define NATOMICS 1000
#endif

int AtomicMain(int argc, char* argv[]) {
  using namespace DrvAPI;
  if (!(myThreadId() == 0 && myCoreId() == 0 && myPodId() == 0 && myPXNId() == 0)) {
    return 0;
  }
  DrvAPIVAddress target_v(&target);
  printf("target = %s\n", target_v.to_string().c_str());
  DrvAPIAddress addr = &target;
  for (int i = 0; i < NATOMICS; i++) {
    memop(&target);
    if (i % 1024 == 0) {
      printf("read %4d of %4d\r", i, NATOMICS);
    }
  }
  return 0;
}

declare_drv_api_main(AtomicMain);
