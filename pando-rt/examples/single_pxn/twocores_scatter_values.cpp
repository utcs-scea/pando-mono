// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cassert>
#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file twocores_scatter_values.cpp
 * This is a simple scattering test.
 * The first core scatters values to the first half (1/num_cores) cores,
 * and the last core scatter values to the remaining half cores, on the same PXN
 * and POD.
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
 * @brief Cores (0,0) and (dim.x-1, dim.y-1) scatter integer values
 * to other cores, half and half from each source core, on the same
 * PXN and the same POD.
 */
void scatterValues(pando::GlobalPtr<std::int64_t> sharedArray, bool isFirstCore,
                   std::int8_t numCores) {
  const auto thisPlace = pando::getCurrentPlace();
  const auto placeDims = pando::getPlaceDims();
  if (!(thisPlace.core.x == 0 && thisPlace.core.y == 0) &&
      !((placeDims.core.x - 1) == thisPlace.core.x && (placeDims.core.y - 1) == thisPlace.core.y)) {
    std::printf("Incorrect core attempts to scatter\n");
    pando::exit(EXIT_FAILURE);
  }

  // Increment its own value.
  sharedArray[placeDims.core.y * thisPlace.core.y + thisPlace.core.x] = solution;

  std::int8_t numAssignedCores = (isFirstCore) ? (numCores / 2) : (numCores - (numCores / 2));
  std::int8_t ix = thisPlace.core.x;
  std::int8_t iy = thisPlace.core.y;
  // Scatter values over cores that this core manage.
  for (std::int8_t i = 0; i < numAssignedCores; ++i) {
    if (i > 0) {
      std::printf("core (%i,%i) scatters a value to core (%i,%i)\n", thisPlace.core.x,
                  thisPlace.core.y, ix, iy);
      pando::CoreIndex otherCore{ix, iy};
      pando::Place otherCorePlace{thisPlace.node, thisPlace.pod, otherCore};
      std::int16_t offset = (placeDims.core.y * iy) + ix;
      sharedArray[offset] = solution - 1;
      PANDO_CHECK(pando::executeOn(otherCorePlace, &incrementValue, &sharedArray[offset]));
    }
    if (isFirstCore) {
      ++ix;
      if (ix == placeDims.core.x) {
        ix = 0;
        ++iy;
        assert(iy < placeDims.core.y);
      }
      assert(ix < placeDims.core.x);
    } else {
      --ix;
      if (ix < 0) {
        ix = placeDims.core.x - 1;
        --iy;
        assert(iy >= 0);
      }
      assert(ix > 0);
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

  if (placeDims.core.x * placeDims.core.y < 2) {
    std::printf("# core should be >= 2");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0");
    pando::exit(EXIT_FAILURE);
  }

  const auto thisPlace = pando::getCurrentPlace();
  std::int16_t numCores = placeDims.core.x * placeDims.core.y;
  auto mmResource = pando::getDefaultMainMemoryResource();
  auto sharedArray = static_cast<pando::GlobalPtr<std::int64_t>>(
      mmResource->allocate(sizeof(std::int64_t) * numCores));

  if (thisPlace.node.id == 0) {
    pando::CoreIndex firstCore{0, 0};
    pando::CoreIndex lastCore{static_cast<std::int8_t>(placeDims.core.x - 1),
                              static_cast<std::int8_t>(placeDims.core.y - 1)};
    pando::Place firstCorePlace{thisPlace.node, thisPlace.pod, firstCore};
    pando::Place lastCorePlace{thisPlace.node, thisPlace.pod, lastCore};
    PANDO_CHECK(pando::executeOn(firstCorePlace, &scatterValues, sharedArray, true, numCores));
    PANDO_CHECK(pando::executeOn(lastCorePlace, &scatterValues, sharedArray, false, numCores));
  }

  pando::waitAll();

  correctnessCheck(sharedArray);
  mmResource->deallocate(sharedArray, sizeof(std::int64_t) * numCores);

  return 0;
}
