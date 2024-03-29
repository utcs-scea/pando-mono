// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdio>

int ArgvMain(int argc, char* argv[]) {
  printf("Hello from %s\n", __PRETTY_FUNCTION__);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  return 0;
}

declare_drv_api_main(ArgvMain);
