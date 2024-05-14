// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cassert>
#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file allcores_reducebroadcast_values.cpp
 *  In this example, all tasks are invoked by a single core on PXN-0.
 *  This example performs the following steps:
 *  1) Each PXN allocates and initializes a local array.
 *  2) Each PXN is assigned an index range and becomes an owner of the elements within that range.
 *    - Element `i` assignment policy: (i % pando::getPlaceDims()) is the owner PXN ID.
 *  3) All cores of each PXN aggregate and combine (reduce) owning values from other PXNs.
 *    - This uses amorphous data parallelism; so each core acquires each index and its element,
 *      loads elements of the index on remote arrays, and combines them in parallel.
 *  4) All cores of each PXN broadcast the combined (reduced) owning values to other PXNs.
 *    - This uses amorphous data parallelism; so each core acquires each index and its element,
 *      and stores elements of the index on remote arrays in parallel.
 */

constexpr std::int64_t initialValue{1};

/**
 * @brief Wait until invoked tasks complete.
 */
template <typename T, typename S>
void waitUntil(pando::GlobalPtr<T> dones, T expected, S size) {
  for (S n = 0; n < size; ++n) {
    pando::waitUntil([dones, expected, n]() {
      return (dones[n] == expected);
    });
    dones[n] = T{}; // Reset this to reuse.
  }
}

/**
 * @brief One core initializes a single element value on `localArray.`
 */
void setValue(pando::GlobalPtr<std::int64_t> localArray, std::int64_t value, std::int32_t index,
              pando::GlobalPtr<bool> coreDone) {
  localArray[index] = value;
  *coreDone = true;
}

/**
 * @brief Set a local array of the current PXN by `value.`
 */
void initializeLocalArray(pando::GlobalPtr<std::int64_t> localArray, std::int64_t value,
                          std::int32_t numCoresPerNode, pando::GlobalPtr<bool> done,
                          [[maybe_unused]] pando::GlobalPtr<bool> coresDone) {
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  // Parallelize local array initialization
  for (std::int32_t c = 0; c < numCoresPerNode; ++c) {
    coresDone[c] = false;
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{thisNodeId}, pando::anyPod, pando::anyCore},
                          &setValue, localArray, value, c, &coresDone[c]));
  }
  // Wait until all core computation completes
  waitUntil(coresDone, true, numCoresPerNode);
  *done = true;
}

/**
 * @brief This function initializes a distributed array by `initialValue`.
 * To do this, each PXN initializes its local array in parallel.
 */
void initializeValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                      std::int32_t numCoresPerNode, pando::GlobalPtr<bool> done,
                      pando::GlobalPtr<pando::GlobalPtr<bool>>& distCoreDones) {
  const auto placeDims = pando::getPlaceDims();
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  assert(thisNodeId == 0);

  // Each PXN initializes its local array by `initialValue`.
  for (std::int64_t ipxn = 0; ipxn < placeDims.node.id; ++ipxn) {
    pando::GlobalPtr<std::int64_t> localArray = distArray[ipxn];
    pando::GlobalPtr<bool> localCoreDone = distCoreDones[ipxn];
    if (ipxn == thisNodeId) {
      initializeLocalArray(localArray, initialValue, numCoresPerNode, &done[ipxn], localCoreDone);
    } else {
      pando::Place remotePlace{pando::NodeIndex{ipxn}, pando::anyPod, pando::anyCore};
      PANDO_CHECK(pando::executeOn(remotePlace, &initializeLocalArray, localArray, initialValue,
                                   numCoresPerNode, &done[ipxn], localCoreDone));
    }
  }
}

/**
 * @brief A core reduces a value.
 */
void reduceValue(pando::GlobalPtr<std::int64_t> ownArray, std::int64_t srcValue, std::int32_t index,
                 pando::GlobalPtr<bool> coreDone) {
  ownArray[index] = ownArray[index] + (pando::getCurrentPlace().node.id * srcValue);
  coreDone[index] = true;
}

/**
 * @brief Each PXN performs sum-reduction over its owning elements;
 * PXN-(i % the total number of PXNs) owns i-th elements on each local array.
 */
void reduceOwnValuesParallel(pando::GlobalPtr<std::int64_t> srcArray,
                             pando::GlobalPtr<std::int64_t> ownArray, std::int32_t numCoresPerNode,
                             pando::GlobalPtr<bool> dones, pando::GlobalPtr<bool> coresDone) {
  const auto placeDims = pando::getPlaceDims();
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  for (std::int32_t i = 0; i < numCoresPerNode; ++i) {
    coresDone[i] = false;
    if (i % placeDims.node.id == thisNodeId) {
      PANDO_CHECK(pando::executeOn(
          pando::Place{pando::NodeIndex{thisNodeId}, pando::anyPod, pando::anyCore}, &reduceValue,
          ownArray, static_cast<std::int64_t>(srcArray[i]), i, coresDone));
    } else {
      coresDone[i] = true;
    }
  }
  waitUntil(coresDone, true, numCoresPerNode);
  dones[thisNodeId] = true;
}

/**
 * @brief One PXN invokes value sum-reduction for each PXN.
 * Then, each PXN performs sum-reduction over its owning elements;
 * PXN-(i % the total number of PXNs) owns i-th elements on each local array.
 */
void reduceValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                  std::int32_t numCoresPerNode, pando::GlobalPtr<bool> boolDones,
                  pando::GlobalPtr<pando::GlobalPtr<bool>> distCoreDones) {
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
      pando::GlobalPtr<bool> dstCoreDone = distCoreDones[dpxn];

      if (dpxn == pando::getCurrentPlace().node.id) {
        reduceOwnValuesParallel(srcArray, dstArray, numCoresPerNode, boolDones, dstCoreDone);
      } else {
        pando::Place ownerPlace{pando::NodeIndex{dpxn}, pando::anyPod, pando::anyCore};
        PANDO_CHECK(pando::executeOn(ownerPlace, &reduceOwnValuesParallel, srcArray, dstArray,
                                     numCoresPerNode, boolDones, dstCoreDone));
      }
    }
    // This requires for correctness; FGMT:core is a M:N mapping, and so, without this
    // synchronization, multiple tasks could be executed on a single core in parallel.
    waitUntil(boolDones, true, placeDims.node.id);
  }
}

/**
 * @brief Each PXN broadcasts its owning element to remote arrays.
 */
void broadcastOwnValues(pando::GlobalPtr<std::int64_t> srcArray,
                        pando::GlobalPtr<std::int64_t> ownArray, std::int32_t numCoresPerNode,
                        pando::GlobalPtr<bool> dones, pando::GlobalPtr<bool> coresDone) {
  const auto placeDims = pando::getPlaceDims();
  std::int64_t thisNodeId = pando::getCurrentPlace().node.id;
  for (std::int32_t i = 0; i < numCoresPerNode; ++i) {
    if (i % placeDims.node.id == thisNodeId) {
      // Copying a value wrapped by `GlobalPtr` requires casting,
      // since otherwise, it copies `GlobalPtr::Proxy` itself.
      PANDO_CHECK(
          executeOn(pando::Place{pando::NodeIndex{thisNodeId}, pando::anyPod, pando::anyCore},
                    &setValue, srcArray, static_cast<std::int64_t>(ownArray[i]), i, &coresDone[i]));
    } else {
      coresDone[i] = true;
    }
  }
  waitUntil(coresDone, true, numCoresPerNode);
  dones[thisNodeId] = true;
}

/**
 * @brief One PXN invokes broadcasting operations for each PXN.
 * Then, each PXN broadcasts its owning elements to other PXNs;
 * PXN-(i % the total number of PXNs) owns i-th elements on each local array.
 */
void broadcastValues(pando::GlobalPtr<pando::GlobalPtr<std::int64_t>>& distArray,
                     std::int32_t numCoresPerNode, pando::GlobalPtr<bool> boolDones,
                     pando::GlobalPtr<pando::GlobalPtr<bool>> distCoreDones) {
  const auto placeDims = pando::getPlaceDims();
  // Iterate a pair of PXNs, and pass all the remote arrays to each
  // PXN. Then, each PXN broadcasts its owning elements to other PXNs.
  for (std::int64_t spxn = 0; spxn < placeDims.node.id; ++spxn) {
    pando::GlobalPtr<std::int64_t> srcArray = distArray[spxn];
    for (std::int64_t dpxn = 0; dpxn < placeDims.node.id; ++dpxn) {
      if (spxn == dpxn) {
        boolDones[spxn] = true;
        continue;
      }
      pando::GlobalPtr<std::int64_t> dstArray = distArray[dpxn];
      pando::GlobalPtr<bool> dstCoreDone = distCoreDones[dpxn];
      pando::Place ownerPlace{pando::NodeIndex{dpxn}, pando::anyPod, pando::anyCore};
      PANDO_CHECK(pando::executeOn(ownerPlace, &broadcastOwnValues, srcArray, dstArray,
                                   numCoresPerNode, boolDones, dstCoreDone));
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
      std::int64_t ownerPXN = (c % placeDims.node.id);
      if (localArray[c] != std::int64_t{ownerPXN * (placeDims.node.id - 1) + 1}) {
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
    // This array is used to synchronize intra-PXN parallelism
    pando::GlobalPtr<pando::GlobalPtr<bool>> distCoreDones =
        static_cast<pando::GlobalPtr<pando::GlobalPtr<bool>>>(
            mmResource->allocate(sizeof(bool) * numPXNs));
    // Remote and local distributed array memory allocation
    // Remote and local distributed core done checking array memory allocation
    for (std::int64_t n = 0; n < numPXNs; ++n) {
      const pando::Place otherPlace{pando::NodeIndex{n}, pando::anyPod, pando::anyCore};

      {
        auto result = pando::allocateMemory<std::int64_t>(numCoresPerNode, otherPlace,
                                                          pando::MemoryType::Main);
        if (!result.hasValue()) {
          std::printf("Failed to allocate memory.\n");
          pando::exit(EXIT_FAILURE);
        }
        distArray[n] = result.value();
      }

      {
        auto result =
            pando::allocateMemory<bool>(numCoresPerNode, otherPlace, pando::MemoryType::Main);
        if (!result.hasValue()) {
          std::printf("Failed to allocate memory.\n");
          pando::exit(EXIT_FAILURE);
        }
        distCoreDones[n] = result.value();
      }
    }
    // Scatter values to remote PXNs
    initializeValues(distArray, numCoresPerNode, boolDones, distCoreDones);
    // Wait until initialization completes
    waitUntil(boolDones, true, numPXNs);
    reduceValues(distArray, numCoresPerNode, boolDones, distCoreDones);
    broadcastValues(distArray, numCoresPerNode, boolDones, distCoreDones);
    correctnessCheck(distArray, numCoresPerNode);

    // Deallocate arrays
    for (std::int64_t n = 0; n < placeDims.node.id; ++n) {
      pando::GlobalPtr<std::int64_t> localArray = distArray[n];
      pando::deallocateMemory(localArray, sizeof(std::int64_t) * numCoresPerNode);
      pando::GlobalPtr<bool> localCoreDone = distCoreDones[n];
      pando::deallocateMemory(localCoreDone, sizeof(bool) * numCoresPerNode);
    }

    mmResource->deallocate(distArray, sizeof(pando::GlobalPtr<std::int64_t>) * numPXNs);
    mmResource->deallocate(boolDones, sizeof(bool) * numPXNs);
  }
  pando::waitAll();

  return 0;
}
