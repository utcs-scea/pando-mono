// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <math.h>
#include <pandohammer/mmio.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char message[] = "Hello, world!\n";

int main() {
  ph_print_hex(&message);
  return 0;
}
