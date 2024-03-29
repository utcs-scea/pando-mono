// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <machine/syscall.h>
        .global _start
_start:
        li      x1, -1
        li      x2, 2
        slli    x3, x2, 62
        mul     x4, x1, x2
        mulh    x5, x1, x2
        // 2*INT64_MIN
        mul     x6, x3, x2
        mulh    x7, x3, x2
        // 2*UNT64_MAX
        mulhu   x8, x3, x2
        // 2*UINT64_MAX
        mulhsu  x9, x3, x2
        // should trap
        //div     x10, x1, x0
        // should be 1
        div     x10, x1, x1
done:
        li      a7, SYS_exit
        ecall
