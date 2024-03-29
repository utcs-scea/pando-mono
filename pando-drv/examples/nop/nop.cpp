// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <inttypes.h>
using namespace DrvAPI;

int NopMain(int argc, char* argv[]) {
  int cycles = 1;
  std::string cycles_str = "";
  int arg = 1;
  if (arg < argc) {
    cycles_str = argv[arg++];
    cycles = std::stoi(cycles_str);
  }

  float GHz = HZ() / 1e9;
  uint64_t ns = cycles / GHz;

  printf("Thread %d on core %d: cycle=%" PRIu64 " invoking nop for %d cycles (%" PRIu64 " ns)\n",
         myThreadId(), myCoreId(), cycle(), cycles, ns);

  DrvAPI::nop(cycles);

  printf("Thread %d on core %d: cycle=%" PRIu64 " completed nop\n", myThreadId(), myCoreId(),
         cycle());

  printf("Thread %d on core %d: cycle=%" PRIu64 " hz = %" PRIu64 "\n", myThreadId(), myCoreId(),
         cycle(), HZ());
  return 0;
}

declare_drv_api_main(NopMain);
