// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdio>

int SimpleMain(int argc, char *argv[]) {
    printf("Simple hello world\n");
    return 0;
}

declare_drv_api_main(SimpleMain);
