// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <string.h>

/**
 * print_int.c
 */
static inline void print_int(long x)
{
    *(volatile long*)0xFFFFFFFFFFFF0000 = x;
}

static inline void print_hex(unsigned long x)
{
    *(volatile unsigned long*)0xFFFFFFFFFFFF0008 = x;
}

static inline void print_char(char x)
{
    *(volatile char*)0xFFFFFFFFFFFF0010 = x;
}

#define ARRAY_SIZE(x) \
    (sizeof(x)/sizeof((x)[0]))

//__attribute__((section(".dram")))
char message[] = "Hello, world!\n";

int main()
{
    for (int i = 0; i < strlen(message); i++) {
        print_char(message[i]);
    }
    return 0;
}
