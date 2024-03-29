// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>

int my_printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return write(STDOUT_FILENO, buf, strlen(buf));
}

void pfield(const char* type, bool is_signed, size_t size) {
  my_printf("%s\tsigned %s\t size, %2zu\n", type, is_signed ? "yes" : "no", size);
}

#define field(type) pfield(#type, std::is_signed<type>::value, sizeof(type))

int main(int argc, char* argv[]) {
  struct stat st;
  my_printf("sizeof(st) = %zu\n", sizeof(st));
  field(dev_t);
  field(ino_t);
  field(mode_t);
  field(nlink_t);
  field(uid_t);
  field(gid_t);
  field(dev_t);
  field(off_t);
  field(struct timespec);
  field(struct timespec);
  field(struct timespec);
  field(blksize_t);
  field(blkcnt_t);
  my_printf("sizeof st.st_atim = %zu\n", sizeof(st.st_atim));
  my_printf("sizeof st.st_blksize = %zu\n", sizeof(st.st_blksize));
  my_printf("sizeof st.st_spare4 = %zu\n", sizeof(st.st_spare4));
  return 0;
}
