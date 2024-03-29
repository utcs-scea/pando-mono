// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>
#include <stdint.h>

static uint64_t* table = (uint64_t*)DRAM_BASE;
static int thread_updates = (int)THREAD_UPDATES;
static uint64_t table_size = (uint64_t)TABLE_SIZE;

static uint64_t random(uint64_t* seed) {
  uint64_t x = *seed;
  x ^= x << 13;
  x ^= ~x >> 7;
  x ^= x << 17;
  *seed = x;
  return x;
}

int main() {
  uint64_t seed = myThreadId() + myCoreThreads() * myCoreId() +
                  myCoreThreads() * numPodCores() * myPodId() +
                  myCoreThreads() * numPodCores() * numPXNPods() * myPXNId();

  for (int u = 0; u < thread_updates; u++) {
    uint64_t index = random(&seed) % table_size;
    uint64_t* addr = &table[index];
    // ph_print_hex((unsigned long)addr);
    uint64_t value = *addr;
    value ^= (uint64_t)addr;
    *addr = value;
  }
  return 0;
}
