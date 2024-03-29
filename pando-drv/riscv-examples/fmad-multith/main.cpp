// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <atomic>
#include <cstring>
#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>

#define __l1sp__ __attribute__((section(".dmem")))
#define __l2sp__ \
  __attribute__((section(".dram"))) // TODO: this is actually l2sp; need fix in linker script

__l2sp__ std::atomic<int64_t> barrier = 0;

const uint64_t num_elems = 8;
float a[num_elems] = {5, 5, 5, 5, 5, 5, 5, 5};
float b[num_elems] = {10, 10, 10, 10, 10, 10, 10, 10};
float c[num_elems] = {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5};

char done_msg[] = "all done\n";
char not_done_msg[] = "not done\n";

void update(uint64_t id) {
  c[id] += a[id] * b[id];
  ph_print_float(c[id]);
  barrier.fetch_add(1, std::memory_order_relaxed);
}

int main() {
  uint64_t tid = (myCoreId() << 4) + myThreadId();

  if (tid < num_elems) {
    update(tid);
  }

  if (tid == 0) {
    while (barrier.load(std::memory_order_relaxed) != num_elems) {
      ph_puts(not_done_msg);
    }
    ph_puts(done_msg);

    for (int i = 0; i < num_elems; i++) {
      ph_print_float(c[i]);
    }
  }
}
