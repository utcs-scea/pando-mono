// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main() {
  char filename[] = "test.txt";
  mode_t mode = 0644;
  int flags = O_WRONLY | O_CREAT | O_TRUNC;

  char buf[1024];
  snprintf(buf, sizeof(buf), "open(%s, %x, %d)\n", filename, flags, mode);
  write(STDOUT_FILENO, buf, strlen(buf));

  int fd = open(filename, flags, mode);
  if (fd == -1) {
    char error[1024];
    char errstr[256];
    strerror_r(errno, errstr, sizeof(errstr));
    snprintf(error, sizeof(error) - 1, "Error opening file: %s: %s\n", filename, errstr);
    write(STDOUT_FILENO, error, strlen(error));
    return 1;
  }

  char message[] = "Hello, world!\n";
  write(fd, message, sizeof(message) - 1);
  return 0;
}
