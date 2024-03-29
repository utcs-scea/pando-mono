// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <unistd.h>
int main(int argc, char* argv[]) {
  write(STDOUT_FILENO, "Hello, world!\n", 14);
  return 0;
}
