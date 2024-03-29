// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
using namespace DrvAPI;

DrvAPIGlobalDRAM<int> counter;

#define pr_info(fmt, ...)                                                          \
  do {                                                                             \
    printf("core %2d, thread %2d: " fmt, myCoreId(), myThreadId(), ##__VA_ARGS__); \
  } while (0)

int AmoaddMain(int argc, char* argv[]) {
  DrvAPIAddress addr = &counter;

  pr_info("adding 1\n");
  int r = 0;
  r = DrvAPI::atomic_add<uint64_t>(addr, 1);
  pr_info("read %2d after amoadd\n", r);

  while ((r = DrvAPI::read<uint64_t>(addr)) < 2)
    pr_info("waiting for all cores: (%2d)\n", r);

  return 0;
}

declare_drv_api_main(AmoaddMain);
