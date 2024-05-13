// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file pass_value_in_allcores.cpp
 * This is a simple broadcasting test.
 * One core increments an integer value, and
 * all the other cores on the current environment
 * read that.
 */

constexpr std::int64_t solution{5};

/**
 * @brief Read an integer value that has been increased by core 0.
 */
void readValue(pando::GlobalPtr<std::int64_t> sharedValue, pando::GlobalPtr<bool> checkSolution) {
  if (*sharedValue == solution) {
    auto thisPlace = pando::getCurrentPlace();
    std::printf("[node %li, pod x=%i,y=%i, core x=%i,y=%i] read value: %li\n", thisPlace.node.id,
                thisPlace.pod.x, thisPlace.pod.y, thisPlace.core.x, thisPlace.core.y,
                static_cast<std::int64_t>(*sharedValue));
    (*checkSolution) = true;
  }
}

/**
 * @brief Core 0 increases an integer value and broadcasts to the
 * adjacent right and bottom cores.
 */
void incrementValue(pando::GlobalPtr<std::int64_t> sharedValue,
                    pando::GlobalPtr<bool> checkSolution) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto placeDims = pando::getPlaceDims();
  ++(*sharedValue);

  for (std::int8_t ix = 0; ix < placeDims.core.x; ++ix) {
    for (std::int8_t iy = 0; iy < placeDims.core.y; ++iy) {
      pando::CoreIndex otherCore{ix, iy};
      pando::Place otherCorePlace{thisPlace.node, thisPlace.pod, otherCore};
      std::int16_t offset = (placeDims.core.y * iy) + ix;
      PANDO_CHECK(
          pando::executeOn(otherCorePlace, &readValue, sharedValue, &checkSolution[offset]));
    }
  }
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  if (placeDims.core.x == 0 || placeDims.core.y == 0) {
    std::printf("# core should be > 0");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  const auto thisPlace = pando::getCurrentPlace();
  std::int16_t numCores = placeDims.core.x * placeDims.core.y;
  auto mmResource = pando::getDefaultMainMemoryResource();
  auto sharedValue =
      static_cast<pando::GlobalPtr<std::int64_t>>(mmResource->allocate(sizeof(std::int64_t)));
  auto checkSolution =
      static_cast<pando::GlobalPtr<bool>>(mmResource->allocate(sizeof(bool) * numCores));
  *sharedValue = solution - 1;

  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &incrementValue, sharedValue, checkSolution));
  }

  pando::waitAll();

  for (std::int16_t c = 0; c < numCores; ++c) {
    if (!checkSolution[c]) {
      std::printf("Failed.\n");
      pando::exit(EXIT_FAILURE);
    }
  }
  std::printf("Succeeded.\n");

  mmResource->deallocate(sharedValue, sizeof(std::int64_t));
  mmResource->deallocate(checkSolution, sizeof(bool) * numCores);

  return 0;
}
