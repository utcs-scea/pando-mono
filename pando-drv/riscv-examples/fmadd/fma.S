// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <machine/syscall.h>
#define ONE f31
#define TWO f30
#define THREE f29
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

        la      a2, _one
        flw     ONE, 0(a2) // _one
        la      a2, _two
        flw     TWO, 0(a2) // _two
        la      a2, _three
        flw     THREE, 0(a2) // _three

        fmadd.s f16, ONE, f0, f0
        fmadd.s f17, TWO, f1, f1
        fmadd.s f18, THREE, f2, f2
        fmadd.s f19, ONE, f3, f2
        // exit
        li      a7, SYS_exit
        ecall
_stop:
        j       _stop
        .option pop
