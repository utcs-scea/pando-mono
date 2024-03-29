// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

//__attribute__((section(".dmem")))
//__attribute__((section(".dram")))
float _src[] = {
    1e30, 1e20, 1e10, 1e0, -1e0, -1e-10, -1e-20, -1e-30,
};

//__attribute__((section(".dmem")))
//__attribute__((section(".dram")))
float _dst[ARRAY_SIZE(_src)];
