// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cassert>
#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file onecore_scatter_diffdim_arrays_values.
 *  In this example, all tasks are invoked by a single core on PXN-0.
 *  This example performs the following steps:
 *  1) Each PXN allocates a local array without initialization.
 *    - Each local array has "different" dimension, and so, PXN-0 also manages the size of each
 * PXN's local array. 2) A single core on PXN-0 scatters and sets a value to local arrays of the
 * reomte PXNs.
 */

constexpr std::int64_t solution{5};

/**
 * @brief Set a local array of the current PXN by `value.`
 */
void setValue(pando::GlobalPtr<std::int64_t> localArray, std::int64_t value,
              pando::GlobalPtr<std::uint64_t> size, pando::GlobalPtr<bool> done) {
  for (std::uint64_t i = 0; i < *size; ++i) {
    localArray[i] = value;
  }
  *done = true;
}

/**
 * @brief A single core scatters an integer value to remote PXNs.
 */
void scatterValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> distHeteroArray,
                   pando::GlobalPtr<std::uint64_t> distHeteroArraySize,
                   pando::GlobalPtr<bool> done) {
  const auto placeDims = pando::getPlaceDims();
  std::int16_t thisNodeId = pando::getCurrentPlace().node.id;
  assert(thisNodeId == 0);

  pando::GlobalPtr<std::int64_t> localArray = distHeteroArray[thisNodeId];
  // Set a local array by a solution
  for (std::uint64_t i = 0; i < distHeteroArraySize[thisNodeId]; ++i) {
    localArray[i] = solution;
  }
  done[thisNodeId] = true;

  // Scatter values to remote PXNs.
  for (std::int16_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::Place destPlace{pando::NodeIndex{ipxn}, pando::anyPod, pando::anyCore};
    pando::GlobalPtr<std::int64_t> remoteArray = distHeteroArray[ipxn];
    PANDO_CHECK(pando::executeOn(destPlace, &setValue, remoteArray, solution,
                                 &distHeteroArraySize[ipxn], &done[ipxn]));
  }
}

/**
 * @brief Check correctness.
 */
void correctnessCheck(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> output,
                      pando::GlobalPtr<std::uint64_t> outputSizes) {
  const auto placeDims = pando::getPlaceDims();
  // Check correctness.
  bool check_correctness{true};
  for (std::int16_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::GlobalPtr<std::int64_t> localArray = output[ipxn];
    for (std::uint64_t i = 0; i < outputSizes[ipxn]; ++i) {
      if (localArray[i] != solution) {
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

/**
 * @brief Wait until invoked tasks complete.
 */
void waitUntil(pando::GlobalPtr<bool> dones, std::int16_t numNodes) {
  for (std::int16_t n = 0; n < numNodes; ++n) {
    pando::waitUntil([dones, n]() {
      return dones[n];
    });
    dones[n] = false; // Reset this to reuse.
  }
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i).\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  if (placeDims.core.x == 0 || placeDims.core.y == 0) {
    std::printf("# core should be > 1; one core is reserved for the runtime.\n");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0.\n");
    pando::exit(EXIT_FAILURE);
  }

  const auto thisPlace = pando::getCurrentPlace();

  if (thisPlace.node.id == 0) {
    std::int16_t numPXNs = placeDims.node.id;
    auto mmResource = pando::getDefaultMainMemoryResource();

    // A global array that holds pointers to PXN-local array is
    // allocated and managed on PXN-0
    pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> distHeteroArray =
        static_cast<pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>>(
            mmResource->allocate(sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs));
    pando::GlobalPtr<std::uint64_t> distHeteroArraySize =
        static_cast<pando::GlobalPtr<std::uint64_t>>(
            mmResource->allocate(sizeof(std::uint64_t) * numPXNs));
    pando::GlobalPtr<bool> dones =
        static_cast<pando::GlobalPtr<bool>>(mmResource->allocate(sizeof(bool) * numPXNs));
    // Remote and local memory allocation.
    // Each PXN has different sized memory.
    for (std::int16_t n = 0; n < placeDims.node.id; ++n) {
      const pando::Place otherPlace{pando::NodeIndex{n}, pando::anyPod, pando::anyCore};
      auto result =
          pando::allocateMemory<std::int64_t>(n + 10, otherPlace, pando::MemoryType::Main);
      if (!result.hasValue()) {
        std::printf("Failed to allocate memory.\n");
        pando::exit(EXIT_FAILURE);
      }
      distHeteroArray[n] = result.value();
    }
    // Scatter values to remote PXNs
    scatterValues(distHeteroArray, distHeteroArraySize, dones);
    // Wait until all scattering completes
    waitUntil(dones, placeDims.node.id);
    correctnessCheck(distHeteroArray, distHeteroArraySize);

    // Deallocate arrays
    for (std::int16_t n = 0; n < placeDims.node.id; ++n) {
      pando::GlobalPtr<std::int64_t> localArray = distHeteroArray[n];
      pando::deallocateMemory(localArray, sizeof(std::int64_t) * distHeteroArraySize[n]);
    }
    mmResource->deallocate(distHeteroArray, sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs);
    mmResource->deallocate(dones, sizeof(bool) * numPXNs);
  }
  pando::waitAll();

  return 0;
}
