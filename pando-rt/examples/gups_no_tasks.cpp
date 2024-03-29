// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <cstdlib>
#include <pando-rt/pando-rt.hpp>

/** @file gups_single_node.cpp
 * GUPS (Giga UPdates per Second) benchmark that matches Drv-X / Drv-R implementation without task
 * spawning.
 */

void gupsMain(pando::GlobalPtr<std::int64_t> tablePtr, std::uint64_t tableSize,
              std::uint64_t threadUpdates) {
  for (std::uint64_t u = 0; u < threadUpdates; ++u) {
    const std::uint64_t i = std::rand() % tableSize;
    const auto addr = tablePtr + i;
    const auto val = *addr;
    *addr = val ^ i;
  }
}

int pandoMain(int argc, char** argv) {
  // TODO(max/ashwin): set table size that is palatable by PREP and DrvX backends
  std::uint64_t tableSize = (8 * 1024 * 1024);
  std::uint64_t threadUpdates = 1024;

  if (argc > 1) {
    tableSize = std::atoll(argv[1]);
  }
  if (argc > 2) {
    threadUpdates = std::atoll(argv[2]);
  }
  if ((argc > 3) || (tableSize == 0) || (threadUpdates == 0)) {
    std::printf("Usage: %s [table size] [updates / thread]\n", argv[0]);
    pando::exit(1);
  }

  std::printf("Table size per node: %lu, updates / thread: %lu\n", tableSize, threadUpdates);

  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%i), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  const auto tableByteCount = tableSize * sizeof(std::int64_t);
  auto memoryResource = pando::getDefaultMainMemoryResource();

  auto tablePtr =
      static_cast<pando::GlobalPtr<std::int64_t>>(memoryResource->allocate(tableByteCount));

  gupsMain(tablePtr, tableSize, threadUpdates);

  memoryResource->deallocate(tablePtr, tableByteCount);

  return 0;
}
