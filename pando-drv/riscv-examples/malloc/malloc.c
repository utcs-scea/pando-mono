// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int *p, *q, *r;
void malloc_test() {
  p = malloc(10 * sizeof(int));
  printf("malloc(10) = %x\n", p);
  q = malloc(20 * sizeof(int));
  printf("malloc(20) = %x\n", q);
  free(p);
  p = malloc(30 * sizeof(int));
  printf("malloc(30) = %x\n", p);
  r = malloc(10 * sizeof(int));
  printf("malloc(10) = %x\n", r);
  return;
}
