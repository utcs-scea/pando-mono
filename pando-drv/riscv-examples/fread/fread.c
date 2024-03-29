// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
  FILE* f = fopen("file.txt", "r");
  if (!f) {
    printf("fopen failed: %s\n", strerror(errno));
    return 1;
  }
  struct stat st;
  int x = fstat(fileno(f), &st);
  if (x < 0) {
    printf("fstat failed: %s\n", strerror(errno));
    return 1;
  }
  printf("st_size = %ld\n", st.st_size);
  char buf[1024];
  int n = fread(buf, 1, sizeof(buf), f);
  if (n < 0) {
    printf("fread failed: %s\n", strerror(errno));
    return 1;
  }
  printf("read %d bytes\n", n);
  printf("buf = \"%s\"", buf);
  fclose(f);

  f = fopen("ofile.txt", "w");
  if (!f) {
    printf("fopen failed: %s\n", strerror(errno));
    return 1;
  }
  fwrite(buf, 1, n, f);
  fclose(f);
  return 0;
}
