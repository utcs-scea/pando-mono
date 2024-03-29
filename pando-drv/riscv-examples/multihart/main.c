// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <string.h>
#include <stdint.h>
#include <pandohammer/mmio.h>
#include <pandohammer/cpuinfo.h>


#define ARRAY_SIZE(x) \
    (sizeof(x)/sizeof((x)[0]))


static inline int hartid() {
    int hart;
    asm volatile ("csrr %0, mhartid" : "=r" (hart));
    return hart;
}

static inline int64_t amoswap(int64_t w, int64_t *p) {
    int64_t r;
    asm volatile ("amoswap.d %0, %1, 0(%2)"
                  : "=r" (r)
                  : "r" (w), "r" (p)
                  : "memory");
    return r;
}

static inline int64_t amoadd(int64_t w, int64_t *p) {
    int64_t r;
    asm volatile ("amoadd.d %0, %1, 0(%2)"
                  : "=r" (r)
                  : "r" (w), "r" (p)
                  : "memory");
    return r;
}

int64_t x = -1;
int64_t y =  0;

int main() {
    ph_print_int(myThreadId());
    ph_print_int(myCoreId());
    ph_print_int(myPodId());
    ph_print_int(myPXNId());
    ph_print_int(myCoreThreads());
    ph_print_int(numPXN());
    ph_print_int(numPodCores());
    ph_print_int(numPXNPods());
    ph_print_int(coreL1SPSize());
    ph_print_int(podL2SPSize());
    ph_print_int(pxnDRAMSize());
    //int64_t id  = hartid();
    //print_int(id);
    // swap id with x
    //print_int(amoswap(id, &x));
    //ph_print_int(amoadd(1, &y));
    return 0;
}
