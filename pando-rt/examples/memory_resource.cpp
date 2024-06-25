// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

void allocate() {
  pando::L2SPResource* l2SPResource = pando::getDefaultL2SPResource();
  pando::MainMemoryResource* mainMemoryResource = pando::getDefaultMainMemoryResource();

  const std::size_t bytes = 8;
  auto pL2SP = l2SPResource->allocate(bytes);
  auto pMainMemory = mainMemoryResource->allocate(bytes);

  l2SPResource->deallocate(pL2SP, bytes);
  mainMemoryResource->deallocate(pMainMemory, bytes);
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  const auto thisPlace = pando::getCurrentPlace();

  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &allocate));
  }

  pando::endExecution();

  return 0;
}
