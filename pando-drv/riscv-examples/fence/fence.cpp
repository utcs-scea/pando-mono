// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>
#include <atomic>
#include <cstdio>
// sections are misnamed at the moment
#define __l1sp__ __attribute__((section(".dmem")))
#define __l2sp__ __attribute__((section(".dram")))


volatile __l1sp__ long l1_done = 0;
volatile __l2sp__ long l2_done = 0;

int main()
{
    if (myThreadId() == 0) {
        l2_done = 1;
        // this ensures that l2_done is visible to thread 1
        // before l1_done is visible to thread 1
        std::atomic_thread_fence(std::memory_order_seq_cst);
        l1_done = 1;
    } else if (myThreadId() == 1) {
        while (true) {
            long l1 = l1_done;
            long l2 = l2_done;
            if (l1 && !l2) {
                printf("FAIL: l1_done is visible to thread 1 before l2_done\n");
                return 1;
            } else if (l2 && !l1) {
                printf("PASS 1/2: l2_done is visible to thread 1 before l1_done\n");
            } else if (l1 && l2) {
                printf("PASS 2/2: l1_done and l2_done are both visible to thread 1\n");
                return 0;
            }
        }
    }
    return 0;
}
