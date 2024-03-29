// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <pandohammer/atomic.h>
#include <pandohammer/mmio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/**
 * printf that doesn't need a lock
 */
int thread_safe_printf(const char* fmt, ...) {
  char buf[256];
  va_list va;
  va_start(va, fmt);
  int ret = vsnprintf(buf, sizeof(buf), fmt, va);
  write(STDOUT_FILENO, buf, strlen(buf));
  va_end(va);
  return ret;
}

int main(int argc, char* argv[]) {
  int64_t* cp_to_ph = (int64_t*)CP_TO_PH_ADDR;
  int64_t* ph_to_cp = (int64_t*)PH_TO_CP_ADDR;
  while (atomic_load_i64(cp_to_ph) != KEY)
    ;
  printf("PH: Received signal from CP\n");
  int64_t i = atomic_fetch_add_i64(ph_to_cp, 1);
  printf("PH: Sent signal to CP (%lld)\n", i);
  return 0;
}
