// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

//__attribute__((section(".dmem")))
//__attribute__((section(".dram")))
float _src[] = {
    1e3, 1e2, 1e1, 1e0, -1e0, -1e-1, -1e-2, -1e-3,
};

//__attribute__((section(".dmem")))
//__attribute__((section(".dram")))
float _one = 1.0;
float _two = 2.0;
float _three = 3.0;
