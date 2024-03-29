// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <machine/syscall.h>
        .section .dmem
        .text
        .global _start
_start:
        .option push
        .option norelax
        li      a0, 4096
        li      a1, 0
_start0:
        beq     a0, a1, _begin
        addi    a1, a1, 1
        j       _start0
_begin:
        la      a1, _src
        flw     f0, 0(a1) // _src[0]
        flw     f1, 4(a1) // _src[1]
        flw     f2, 8(a1) // _src[2]
        flw     f3,12(a1) // _src[3]
        flw     f4,16(a1) // _src[4]
        flw     f5,20(a1) // _src[5]
        flw     f6,24(a1) // _src[6]
        flw     f7,28(a1) // _src[7]
        // store back in reverse order
        la      a2, _dst
        fsw     f0,28(a2) // _dst[7]
        fsw     f1,24(a2) // _dst[6]
        fsw     f2,20(a2) // _dst[5]
        fsw     f3,16(a2) // _dst[4]
        fsw     f4,12(a2) // _dst[3]
        fsw     f5, 8(a2) // _dst[2]
        fsw     f6, 4(a2) // _dst[1]
        fsw     f7, 0(a2) // _dst[0]
        // read back
        flw     f15,28(a2) // _dst[7]
        flw     f14,24(a2) // _dst[6]
        flw     f13,20(a2) // _dst[5]
        flw     f12,16(a2) // _dst[4]
        flw     f11,12(a2) // _dst[3]
        flw     f10, 8(a2) // _dst[2]
        flw     f9,  4(a2) // _dst[1]
        flw     f8,  0(a2) // _dst[0]
        // exit
        li      a7, SYS_exit
        ecall
_stop:
        j       _stop
        .option pop
