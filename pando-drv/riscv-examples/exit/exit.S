// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <machine/syscall.h>
        .global _start
_start:
        li      a7, SYS_exit
        ecall
done:
        j       done
