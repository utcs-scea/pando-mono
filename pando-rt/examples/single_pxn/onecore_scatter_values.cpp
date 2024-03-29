// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file onecore_scatter_values.cpp
 * This is a simple scattering test.
 * The first core scatters values to other cores.
 * Once each core gets the value, it increments by 1.
 */

constexpr std::int64_t solution{5};

/**
 * @brief Increment a value assigned to the current core..
 */
void incrementValue(pando::GlobalPtr<std::int64_t> sharedValue) {
  ++(*sharedValue);
}

/**
 * @brief Core 0 scatters an integer value to other cores on the same
 * PXN and the same POD.
 */
void scatterValues(pando::GlobalPtr<std::int64_t> sharedArray) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto placeDims = pando::getPlaceDims();

  // Initialize values.
  for (std::int8_t ix = 0; ix < placeDims.core.x; ++ix) {
    for (std::int8_t iy = 0; iy < placeDims.core.y; ++iy) {
      sharedArray[iy * placeDims.core.y + ix] = (solution - 1);
    }
  }
  ++sharedArray[0];

  // Scatter values.
  for (std::int8_t ix = 0; ix < placeDims.core.x; ++ix) {
    for (std::int8_t iy = 0; iy < placeDims.core.y; ++iy) {
      // The current code should be skipped.
      if (ix == 0 && iy == 0) {
        continue;
      }

      pando::CoreIndex otherCore{ix, iy};
      pando::Place otherCorePlace{thisPlace.node, thisPlace.pod, otherCore};
      std::int16_t offset = (placeDims.core.y * iy) + ix;
      PANDO_CHECK(pando::executeOn(otherCorePlace, &incrementValue, &sharedArray[offset]));
    }
  }
}

/**
 * @brief Check correctness.
 */
void correctnessCheck(pando::GlobalPtr<std::int64_t> output) {
  const auto placeDims = pando::getPlaceDims();
  // Check correctness.
  bool check_correctness{true};
  for (std::int8_t ix = 0; ix < placeDims.core.x; ++ix) {
    for (std::int8_t iy = 0; iy < placeDims.core.y; ++iy) {
      if (output[ix + iy * placeDims.core.y] != solution) {
        check_correctness = false;
      }
    }
  }
  if (!check_correctness) {
    std::printf("Failed.\n");
    pando::exit(EXIT_FAILURE);
  }
  std::printf("Succeeded.\n");
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%i), (%i,%i), (%i,%i)\n", placeDims.node.id,
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
  std::int16_t numCores = placeDims.core.x * placeDims.core.y;
  auto mmResource = pando::getDefaultMainMemoryResource();
  auto sharedArray = static_cast<pando::GlobalPtr<std::int64_t>>(
      mmResource->allocate(sizeof(std::int64_t) * numCores));

  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &scatterValues, sharedArray));
  }

  pando::waitAll();

  correctnessCheck(sharedArray);
  mmResource->deallocate(sharedArray, sizeof(std::int64_t) * numCores);

  return 0;
}
