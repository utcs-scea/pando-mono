// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cassert>
#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file onecore_scatter_samedim_arrays_values.
 *  In this example, all tasks are invoked by a single core on PXN-0.
 *  This example performs the following steps:
 *  1) Each PXN allocates a local array without initialization.
 *    - Each local array has "same" dimension.
 *  2) A single core on PXN-0 scatters and sets a value to local arrays of the reomte PXNs.
 */

constexpr std::int64_t solution{5};

/**
 * @brief Set a local array of the current PXN by `value.`
 */
void setValue(pando::GlobalPtr<std::int64_t> localArray, std::int64_t value,
              std::int32_t numCoresPerNode, pando::GlobalPtr<bool> done) {
  for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
    localArray[c] = value;
  }
  *done = true;
}

/**
 * @brief A single core scatters an integer value to remote PXNs.
 */
void scatterValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                   std::int32_t numCoresPerNode, pando::GlobalPtr<bool> done) {
  const auto placeDims = pando::getPlaceDims();
  std::int16_t thisNodeId = pando::getCurrentPlace().node.id;
  assert(thisNodeId == 0);

  // Initialize values.
  pando::GlobalPtr<std::int64_t> localArray = distArray[thisNodeId];
  // Set a local array by a solution
  for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
    localArray[c] = solution;
  }
  done[thisNodeId] = true;

  // Scatter values to remote PXNs.
  for (std::int16_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::Place destPlace{pando::NodeIndex{ipxn}, pando::anyPod, pando::anyCore};
    pando::GlobalPtr<std::int64_t> remoteArray = distArray[ipxn];
    PANDO_CHECK(pando::executeOn(destPlace, &setValue, remoteArray, solution, numCoresPerNode,
                                 &done[ipxn]));
  }
}

/**
 * @brief Check correctness.
 */
void correctnessCheck(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> output,
                      std::int32_t numCoresPerNode) {
  const auto placeDims = pando::getPlaceDims();
  // Check correctness.
  bool check_correctness{true};
  for (std::int16_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::GlobalPtr<std::int64_t> localArray = output[ipxn];
    for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
      if (localArray[c] != solution) {
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
    std::int8_t numPodsPerPXNs = placeDims.pod.x * placeDims.pod.y;
    std::int32_t numCoresPerNode = placeDims.core.x * placeDims.core.y * numPodsPerPXNs;
    auto mmResource = pando::getDefaultMainMemoryResource();

    // A global array that holds pointers to PXN-local array is
    // allocated and managed on PXN-0
    pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> distArray =
        static_cast<pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>>(
            mmResource->allocate(sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs));
    pando::GlobalPtr<bool> dones =
        static_cast<pando::GlobalPtr<bool>>(mmResource->allocate(sizeof(bool) * numPXNs));
    // Remote and local memory allocation
    for (std::int16_t n = 0; n < placeDims.node.id; ++n) {
      const pando::Place otherPlace{pando::NodeIndex{n}, pando::anyPod, pando::anyCore};
      auto result =
          pando::allocateMemory<std::int64_t>(numCoresPerNode, otherPlace, pando::MemoryType::Main);
      if (!result.hasValue()) {
        std::printf("Failed to allocate memory.\n");
        pando::exit(EXIT_FAILURE);
      }
      distArray[n] = result.value();
    }
    // Scatter values to remote PXNs
    scatterValues(distArray, numCoresPerNode, dones);
    // Wait until all scattering completes
    waitUntil(dones, placeDims.node.id);
    correctnessCheck(distArray, numCoresPerNode);

    // Deallocate arrays
    for (std::int16_t n = 0; n < placeDims.node.id; ++n) {
      pando::GlobalPtr<std::int64_t> localArray = distArray[n];
      pando::deallocateMemory(localArray, sizeof(std::int64_t) * numCoresPerNode);
    }
    mmResource->deallocate(distArray, sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs);
    mmResource->deallocate(dones, sizeof(bool) * numPXNs);
  }
  pando::waitAll();

  return 0;
}
