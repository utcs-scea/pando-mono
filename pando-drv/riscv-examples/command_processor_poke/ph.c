// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <pandohammer/mmio.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
/**
 * printf that doesn't need a lock
 */
int thread_safe_printf(const char* fmt, ...)
{
    char buf[256];
    va_list va;
    va_start(va, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, va);
    write(STDOUT_FILENO, buf, strlen(buf));
    va_end(va);
    return ret;
}


int main(int argc, char *argv[])
{
    int64_t *cp_to_ph = (int64_t *)CP_TO_PH_ADDR;
    int64_t *ph_to_cp = (int64_t *)PH_TO_CP_ADDR;
    thread_safe_printf("PH: cp_to_ph = %llx, ph_to_cp = %llx\n"
                       ,(uint64_t)cp_to_ph
                       ,(uint64_t)ph_to_cp
                       );
    while (*cp_to_ph == 0);
    thread_safe_printf("PH: Received signal from CP\n");
    *ph_to_cp = 1;
    thread_safe_printf("PH: Sent signal to CP\n");
    return 0;
}
