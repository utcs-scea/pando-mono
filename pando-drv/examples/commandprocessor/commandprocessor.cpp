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
  if (isCommandProcessor()) {
    printf("I am a command processor, argv=");
  } else {
    printf("My CoreId is %d, argv=", myCoreId());
  }
  for (int i = 0; i < argc; i++) {
    printf(" %s", argv[i]);
  }
  printf("\n");
  return 0;
}

declare_drv_api_main(CPMain);
