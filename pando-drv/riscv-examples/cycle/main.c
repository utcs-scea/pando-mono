// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <string.h>
#include <stdint.h>
#include <pandohammer/mmio.h>
#include <pandohammer/cpuinfo.h>


#define __l1sp__ __attribute__((section(".dmem")))
#define __l2sp__ __attribute__((section(".dram")))

__l1sp__ volatile int x;
int main() {
    for (int i = 0; i < READS; i++) {
        int i = x;
        ph_print_int(cycle());
    }
    return 0;
}
