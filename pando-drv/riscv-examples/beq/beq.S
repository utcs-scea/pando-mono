// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

.global _start
_start:
        beq     x0, x0, success
fail:
        li      x1, 1
        j       fail
success:
        li      x1, 0
        j       success
