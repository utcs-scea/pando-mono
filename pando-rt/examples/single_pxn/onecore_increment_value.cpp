// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file onecore_increment_value.cpp
 * In this test, the 0th core increments an integer value by 1.
 */

constexpr std::int64_t solution{5};

void increase(std::int64_t v) {
  ++v;
  if (v != solution) {
    std::printf("Failed.\n");
    pando::exit(EXIT_FAILURE);
  }
  std::printf("Succeeded.\n");
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);
  const auto thisPlace = pando::getCurrentPlace();

  if (placeDims.core.x == 0 || placeDims.core.y == 0) {
    std::printf("# core should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  // execute on this node
  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &increase, (solution - 1)));
  }

  pando::endExecution();

  return 0;
}
