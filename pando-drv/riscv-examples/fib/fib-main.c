// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// 0 1 2 3 4 5 6  7  8  9 10 11  12  13  14 ...
// 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 ...
extern int fib(int n);

int main() {
  int i = 14;
  int r = fib(i);

  printf("fib(%d) = %d\n", i, r);
  return r;
}
