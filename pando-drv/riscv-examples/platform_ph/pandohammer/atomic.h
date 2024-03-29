// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef PANDOHAMMER_ATOMIC_H
#define PANDOHAMMER_ATOMIC_H
#include <stdint.h>
inline int32_t atomic_fetch_add_i32(volatile int32_t *ptr, int32_t val)
{
    int32_t ret;
    asm volatile("amoadd.w %0, %2, 0(%1)" : "=r"(ret): "r"(ptr) , "r"(val));
    return ret;
}

inline int64_t atomic_fetch_add_i64(volatile int64_t *ptr, int64_t val)
{
    int64_t ret;
    asm volatile("amoadd.d %0, %2, 0(%1)" : "=r"(ret): "r"(ptr) , "r"(val));
    return ret;
}
inline int32_t atomic_swap_i32(volatile int32_t *ptr, int32_t val)
{
    int32_t ret;
    asm volatile("amoswap.w %0, %2, 0(%1)" : "=r"(ret): "r"(ptr) , "r"(val));
    return ret;
}

inline int64_t atomic_swap_i64(volatile int64_t *ptr, int64_t val)
{
    int64_t ret;
    asm volatile("amoswap.d %0, %2, 0(%1)" : "=r"(ret): "r"(ptr) , "r"(val));
    return ret;
}

inline int32_t atomic_compare_and_swap_i32(volatile int32_t *ptr,
                                           int32_t oldval,
                                           int32_t newval)
{
    // address to rs1 (= t3 [x28])
    // newval  to rs2 (= t4 [x29])
    // oldval  to rs3 (= t5 [x30])
    // output  to rd  (= t6 [x31])
    int32_t ret;
    asm volatile("mv x28, %1\n"
                 "mv x29, %3\n"
                 "mv x30, %2\n"
                 ".word 0xf1de2fab\n"
                 "mv %0, x31\n"
                 : "=r"(ret)
                 : "r"(ptr), "r"(oldval), "r"(newval)
                 : "x28", "x29", "x30", "x31", "memory");
    return ret;
}

inline int64_t atomic_compare_and_swap_i64(volatile int64_t *ptr,
                                           int64_t oldval,
                                           int64_t newval)
{
    // address to rs1 (= t3 [x28])
    // newval  to rs2 (= t4 [x29])
    // oldval  to rs3 (= t5 [x30])
    // output  to rd  (= t6 [x31])
    int64_t ret;
    asm volatile("mv x28, %1\n"
                 "mv x29, %3\n"
                 "mv x30, %2\n"
                 ".word 0xf1de3fab\n"
                 "mv %0, x31\n"
                 : "=r"(ret)
                 : "r"(ptr), "r"(oldval), "r"(newval)
                 : "x28", "x29", "x30", "x31", "memory");
    return ret;
}

inline int32_t atomic_load_i32(volatile int32_t *ptr)
{
    return *ptr;
}

inline int64_t atomic_load_i64(volatile int64_t *ptr)
{
    return *ptr;
}
#endif
