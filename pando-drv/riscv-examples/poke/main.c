// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <math.h>
#include <string.h>
#include <pandohammer/mmio.h>

inline void store_byte(volatile char *addr, char val)
{
    asm volatile("sb %0, 0(%1)" : : "r"(val), "r"(addr));
}

int main()
{
    char *store_addr = (char *)STORE_ADDR;
    ph_print_hex((unsigned long)store_addr);
    store_byte(store_addr, STORE_VALUE);
    return 0;
}
