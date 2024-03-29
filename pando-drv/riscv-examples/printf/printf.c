// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <pandohammer/atomic.h>
#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ME (myThreadId() + myCoreId() * myCoreThreads())

#ifdef BARRIER
int64_t barrier = 0;
#endif

int main() {
  printf("hello from thread %d\n", ME);

#ifdef BARRIER
  atomic_fetch_add_i64(&barrier, 1);
  while (atomic_load_i64(&barrier) != THREADS) {
    ;
  }
#endif
  return 0;
}
