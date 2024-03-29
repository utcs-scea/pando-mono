// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "pandohammer/mmio.h"

int main()
{
    ph_print_int(1234567890);
    ph_print_hex(0x1234567890ABCDEF);
    ph_print_char('A');
    return 0;
}
