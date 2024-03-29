// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <errno.h>
#include <fcntl.h>
#include <pandohammer/atomic.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BARRIER
int64_t barrier = 0;
#endif

int main() {
  FILE* f = fopen("file.txt", "r");
  printf("f = %p\n", f);
  if (!f) {
    printf("fopen failed: %s\n", strerror(errno));
    return 1;
  }
  int w, x, y, z;
  fscanf(f, "%d %d %d %d", &w, &x, &y, &z);
  printf("w=%d x=%d y=%d z=%d\n", w, x, y, z);
  fclose(f);
#ifdef BARRIER
  atomic_fetch_add_i64(&barrier, 1);
  while (atomic_load_i64(&barrier) != THREADS) {
    ;
  }
#endif
  return 0;
}
