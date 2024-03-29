// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#pragma once
#define pr_info(fmt, ...)                                                            \
  do {                                                                               \
    printf("PXN %3d: POD: %3d: CORE %3d: " fmt "", myPXNId(), myPodId(), myCoreId(), \
           ##__VA_ARGS__);                                                           \
  } while (0)

extern "C" int pandoMain(int argc, char* argv[]);
