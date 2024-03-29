// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <machine/syscall.h>
        .global _start
_start:
        li      x1, -1
        srli    x1, x1, 32
        li      x2, 0
        li      x3, 0
        add     x2, x2, x1
        addw    x3, x3, x1
        li      a7, SYS_exit
        ecall
done:
        j done
