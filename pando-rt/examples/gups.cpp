// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <cstdlib>
#include <pando-rt/pando-rt.hpp>

/** @file gups.cpp
 * GUPS (Giga UPdates per Second) benchmark that matches Drv-X / Drv-R implementation.
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

  std::printf("Table size: %lu, updates / thread: %lu\n", tableSize, threadUpdates);

  const auto thisPlace = pando::getCurrentPlace();
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  const auto tableByteCount = tableSize * sizeof(std::int64_t);
  pando::GlobalPtr<std::int64_t> tablePtr;
  auto memoryResource = pando::getDefaultMainMemoryResource();
  if (thisPlace.node.id == 0) {
    tablePtr =
        static_cast<pando::GlobalPtr<std::int64_t>>(memoryResource->allocate(tableByteCount));

    for (std::int64_t nodeId = 0; nodeId < placeDims.node.id; ++nodeId) {
      for (std::int8_t podX = 0; podX < placeDims.pod.x; ++podX) {
        for (std::int8_t podY = 0; podY < placeDims.pod.y; ++podY) {
          for (std::int8_t coreX = 0; coreX < placeDims.core.x; ++coreX) {
            for (std::int8_t coreY = 0; coreY < placeDims.core.y; ++coreY) {
              const pando::Place place{pando::NodeIndex{nodeId}, pando::PodIndex{podX, podY},
                                       pando::CoreIndex{coreX, coreY}};
              PANDO_CHECK(pando::executeOn(place, &gupsMain, tablePtr, tableSize, threadUpdates));
            }
          }
        }
      }
    }
  }
  pando::waitAll();

  if (thisPlace.node.id == 0) {
    memoryResource->deallocate(tablePtr, tableByteCount);
  }
  pando::endExecution();

  return 0;
}
