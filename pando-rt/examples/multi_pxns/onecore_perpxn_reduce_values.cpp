// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cassert>
#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file onecore_perpxn_reduce_values.cpp
 *  In this example, all tasks are invoked by a single core on PXN-0.
 *  This example performs the following steps:
 *  1) Each PXN allocates and initializes a local array.
 *  2) Each PXN is assigned an index range and becomes an owner of the elements within that range.
 *    - Element `i` assignment policy: (i % pando::getPlaceDims()) is the owner PXN ID.
 *  3) A single core for each PXN aggregates and combines (reduces) owning values from other PXNs.
 */

constexpr std::int64_t initialValue{1};

/**
 * @brief Wait until invoked tasks complete.
 */
template <typename T>
void waitUntil(pando::GlobalPtr<T> dones, T expected, std::int64_t numNodes) {
  for (std::int64_t n = 0; n < numNodes; ++n) {
    pando::waitUntil([dones, expected, n]() {
      return (dones[n] == expected);
    });
    dones[n] = T{}; // Reset this to reuse.
  }
}

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
 * @brief This function initializes a distributed array by `initialValue`.
 * To do this, each PXN initializes its local array.
 */
void initializeValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                      std::int32_t numCoresPerNode, pando::GlobalPtr<bool> done) {
  const auto placeDims = pando::getPlaceDims();
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  assert(thisNodeId == 0);

  // Each PXN initializes its local array by `initialValue`.
  for (std::int64_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::Place remotePlace{pando::NodeIndex{ipxn}, pando::anyPod, pando::anyCore};
    pando::GlobalPtr<std::int64_t> remoteArray = distArray[ipxn];
    PANDO_CHECK(pando::executeOn(remotePlace, &setValue, remoteArray, initialValue, numCoresPerNode,
                                 &done[ipxn]));
  }
  pando::GlobalPtr<std::int64_t> localArray = distArray[thisNodeId];
  for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
    localArray[c] = initialValue;
  }
  done[thisNodeId] = true;
}

/**
 * @brief Each PXN performs sum-reduction over its owning elements;
 * PXN-(i % the total number of PXNs) owns i-th elements on each local array.
 */
void reduceOwnValues(pando::GlobalPtr<std::int64_t> srcArray,
                     pando::GlobalPtr<std::int64_t> ownArray, std::int32_t numCoresPerNode,
                     pando::GlobalPtr<bool> dones) {
  const auto placeDims = pando::getPlaceDims();
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  for (std::int32_t i = 0; i < numCoresPerNode; ++i) {
    if (i % placeDims.node.id == thisNodeId) {
      ownArray[i] = ownArray[i] + srcArray[i];
    }
  }
  dones[thisNodeId] = true;
}

/**
 * @brief One PXN invokes value sum-reduction for each PXN.
 * Then, each PXN performs sum-reduction over its owning elements;
 * PXN-(i % the total number of PXNs) owns i-th elements on each local array.
 */
void reduceValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                  std::int32_t numCoresPerNode, pando::GlobalPtr<bool> boolDones) {
  const auto placeDims = pando::getPlaceDims();
  // Iterate a pair of PXNs, and pass all the remote arrays to each
  // PXN. Then, each PXN performs sum-reduction to its local array.
  for (std::int64_t spxn = 0; spxn < placeDims.node.id; ++spxn) {
    pando::GlobalPtr<std::int64_t> srcArray = distArray[spxn];
    for (std::int64_t dpxn = 0; dpxn < placeDims.node.id; ++dpxn) {
      if (spxn == dpxn) {
        boolDones[spxn] = true;
        continue;
      }
      pando::GlobalPtr<std::int64_t> dstArray = distArray[dpxn];
      pando::Place ownerPlace{pando::NodeIndex{dpxn}, pando::anyPod, pando::anyCore};
      PANDO_CHECK(pando::executeOn(ownerPlace, &reduceOwnValues, srcArray, dstArray,
                                   numCoresPerNode, boolDones));
    }
    // This requires for correctness; FGMT:core is a M:N mapping, and so, without this
    // synchronization, multiple tasks could be executed on a single core in parallel.
    waitUntil(boolDones, true, placeDims.node.id);
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
  for (std::int64_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::GlobalPtr<std::int64_t> localArray = output[ipxn];
    for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
      if (c % placeDims.node.id == ipxn) {
        // Correct values of the owned elements on the local array
        // are the number of PXNs.
        if (localArray[c] != static_cast<std::int64_t>(placeDims.node.id)) {
          check_correctness = false;
        }
      } else {
        // Correct values of the remote elements on the local array
        // are `initialValue`.
        if (localArray[c] != initialValue) {
          check_correctness = false;
        }
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
    std::int64_t numPXNs = placeDims.node.id;
    std::int8_t numPodsPerPXNs = placeDims.pod.x * placeDims.pod.y;
    std::int32_t numCoresPerNode = placeDims.core.x * placeDims.core.y * numPodsPerPXNs;
    auto mmResource = pando::getDefaultMainMemoryResource();

    // A global array that holds pointers to PXN-local arrays is
    // allocated and managed on PXN-0
    pando::GlobalPtr<pando::GlobalPtr<std::int64_t>> distArray =
        static_cast<pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>>(
            mmResource->allocate(sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs));
    pando::GlobalPtr<bool> boolDones =
        static_cast<pando::GlobalPtr<bool>>(mmResource->allocate(sizeof(bool) * numPXNs));
    // Remote and local memory allocation
    for (std::int64_t n = 0; n < placeDims.node.id; ++n) {
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
    initializeValues(distArray, numCoresPerNode, boolDones);
    // Wait until initialization completes
    waitUntil(boolDones, true, placeDims.node.id);
    reduceValues(distArray, numCoresPerNode, boolDones);
    correctnessCheck(distArray, numCoresPerNode);

    // Deallocate arrays
    for (std::int64_t n = 0; n < placeDims.node.id; ++n) {
      pando::GlobalPtr<std::int64_t> localArray = distArray[n];
      pando::deallocateMemory(localArray, sizeof(std::int64_t) * numCoresPerNode);
    }
    mmResource->deallocate(distArray, sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs);
    mmResource->deallocate(boolDones, sizeof(bool) * numPXNs);
  }
  pando::endExecution();

  return 0;
}
