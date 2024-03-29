// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "machine/syscall.h"
        .global _start
_start:
        li      x1, -1
        sw      x1, 0(x0)
        lwu     x2, 0(x0)
        lbu     x3, 0(x0)
        lbu     x4, 1(x0)
        lbu     x5, 2(x0)
        lbu     x6, 3(x0)
        lhu     x7, 0(x0)
        lhu     x8, 2(x0)
        lhu     x9, 4(x0)
        sb      x1, 8(x0)
        lbu     x10, 8(x0)
        lb      x11, 8(x0)
        sh      x1,  16(x0)
        lhu     x12, 16(x0)
        lh      x13, 16(x0)
        sw      x1,  24(x0)
        lwu     x14, 24(x0)
        lw      x15, 24(x0)
done:
        li      a7, SYS_exit
        ecall
