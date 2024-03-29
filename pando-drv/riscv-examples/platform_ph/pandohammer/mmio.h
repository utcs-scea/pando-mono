// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDOHAMMER_MMIO_H
#define PANDOHAMMER_MMIO_H
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

static inline void ph_print_float(float x) {
  *(volatile float*)0xFFFFFFFFFFFF0000 = x;
}

static inline void ph_print_int(long x) {
  *(volatile long*)0xFFFFFFFFFFFF0000 = x;
}

static inline void ph_print_hex(unsigned long x) {
  *(volatile unsigned long*)0xFFFFFFFFFFFF0008 = x;
}

static inline void ph_print_char(char x) {
  *(volatile char*)0xFFFFFFFFFFFF0010 = x;
}

static inline void ph_puts(char* cstr) {
  for (long i = 0; i < strlen(cstr); i++) {
    ph_print_char(cstr[i]);
  }
}

static inline void ph_print_time() {
  *(volatile char*)0xFFFFFFFFFFFF0018 = 0;
}

#ifdef __cplusplus
}
#endif
#endif
