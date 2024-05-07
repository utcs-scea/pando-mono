// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <cinttypes>

#include <pando-rt/pando-rt.hpp>

/** @file pass_value_in_twocores.cpp
 * This is a simple value passing test.
 * One core increments an integer value, and
 * the right and bottom adjacent cores read that value.
 */

constexpr std::int64_t solution{5};

/**
 * @brief Read an integer value that has been increased by core 0.
 */
void readValue(pando::GlobalPtr<std::int64_t> sharedValue) {
  if (*sharedValue == solution) {
    auto thisPlace = pando::getCurrentPlace();
    std::printf("[node %" PRId64 ", pod x=%i,y=%i, core x=%i,y=%i] read value: %li\n", thisPlace.node.id,
                thisPlace.pod.x, thisPlace.pod.y, thisPlace.core.x, thisPlace.core.y,
                static_cast<std::int64_t>(*sharedValue));
  }
}

/**
 * @brief Core 0 increases an integer value and broadcasts to the
 * adjacent right and bottom cores.
 */
void incrementValue(pando::GlobalPtr<std::int64_t> sharedValue) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto placeDims = pando::getPlaceDims();
  ++(*sharedValue);
  pando::CoreIndex rightCore{static_cast<std::int8_t>((thisPlace.core.x + 1) % placeDims.core.x),
                             thisPlace.core.y};
  // TODO(hc): The current PREP only supports 1D core alignment,
  // and so the bottom core is for now always (0, 0).
  pando::CoreIndex bottomCore{thisPlace.core.x,
                              static_cast<std::int8_t>((thisPlace.core.y + 1) % placeDims.core.y)};
  pando::Place rightCorePlace{thisPlace.node, thisPlace.pod, rightCore};
  pando::Place bottomCorePlace{thisPlace.node, thisPlace.pod, bottomCore};
  PANDO_CHECK(pando::executeOn(rightCorePlace, &readValue, sharedValue));
  PANDO_CHECK(pando::executeOn(bottomCorePlace, &readValue, sharedValue));
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  if (placeDims.core.x == 0 || placeDims.core.y == 0) {
    std::printf("# core should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  const auto thisPlace = pando::getCurrentPlace();
  auto mmResource = pando::getDefaultMainMemoryResource();
  auto sharedValue =
      static_cast<pando::GlobalPtr<std::int64_t>>(mmResource->allocate(sizeof(std::int64_t)));
  *sharedValue = solution - 1;

  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &incrementValue, sharedValue));
  }

  pando::waitAll();

  mmResource->deallocate(sharedValue, sizeof(std::int64_t));

  return 0;
}
