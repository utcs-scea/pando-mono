// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>

using namespace DrvAPI;

DrvAPIGlobalDRAM<int> g_lock;
DrvAPIGlobalDRAM<int> sum;

#define DO_LOCK

int LockMain(int argc, char *argv[]) {
    static constexpr int backoff_liit = 1000;
    int backoff_counter = 8;
#ifdef DO_LOCK
    while (atomic_swap(&g_lock, 1) == 1) {
        DrvAPI::wait(backoff_counter);
        backoff_counter = std::min(backoff_counter*2, backoff_liit);
    }
#endif
    int old_sum = sum;
    printf("sum = %d\n", old_sum);
    sum = old_sum + 1;
#ifdef DO_LOCK
    atomic_swap(&g_lock, 0);
#endif
    return 0;
}

declare_drv_api_main(LockMain);
