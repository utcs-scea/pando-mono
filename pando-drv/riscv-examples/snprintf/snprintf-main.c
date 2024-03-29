// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <unistd.h>
#include <string.h>
#include <stdio.h>
int snprintf_input();
int main()
{
    int x = snprintf_input();
    int y = snprintf_input();
    unsigned z = snprintf_input();
    char buf[256];
    snprintf(buf, sizeof(buf), "x = %d, y = %d, z = 0x%x\n", x, y, z);
    write(STDOUT_FILENO, buf, strlen(buf));
    return 0;
}
