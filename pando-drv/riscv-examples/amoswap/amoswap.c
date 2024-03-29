// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

int x = 0;
int main() {
  int r, v = 1;
  asm volatile("amoswap.w.aqrl %0, %1, 0(%2)" : "=r"(r) : "r"(v), "r"(&x) : "memory");
  return r;
}
