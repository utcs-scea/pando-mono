// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/wmd_graph_importer.hpp>

[[nodiscard]] pando::Status galois::internal::buildVirtualToPhysicalMapping(
    std::uint64_t numHosts,
    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledVirtualCounts,
    pando::GlobalPtr<pando::Array<std::uint64_t>> virtualToPhysicalMapping,
    pando::Array<std::uint64_t> numEdges) {
  pando::Status err;

  std::sort(labeledVirtualCounts.begin(), labeledVirtualCounts.end());

  pando::Array<std::uint64_t> vTPH;
  err = vTPH.initialize(labeledVirtualCounts.size());
  if (err != pando::Status::Success) {
    return err;
  }

  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> intermediateSort;
  err = intermediateSort.initialize(numHosts);
  if (err != pando::Status::Success) {
    vTPH.deinitialize();
    return err;
  }

  for (std::uint64_t i = 0; i < numHosts; i++) {
    intermediateSort[i] =
        galois::Pair<std::uint64_t, std::uint64_t>{static_cast<std::uint64_t>(0), i};
    numEdges[0] = 0;
  }

  for (auto it = labeledVirtualCounts.rbegin(); it != labeledVirtualCounts.rend(); it++) {
    galois::Pair<std::uint64_t, std::uint64_t> virtualPair = *it;
    // Find minimum
    std::sort(intermediateSort.begin(), intermediateSort.end(),
              [](const galois::Pair<std::uint64_t, std::uint64_t> a,
                 const galois::Pair<std::uint64_t, std::uint64_t> b) {
                return a.first < b.first;
              });
    // Get Minimum
    galois::Pair<std::uint64_t, std::uint64_t> physicalPair = intermediateSort[0];
    // Store the id mappings
    vTPH[virtualPair.second] = physicalPair.second;
    // Update the count
    physicalPair.first += virtualPair.first;
    numEdges[physicalPair.second] = physicalPair.first;
    // Store back
    intermediateSort[0] = physicalPair;
  }

  intermediateSort.deinitialize();

  *virtualToPhysicalMapping = vTPH;
  return pando::Status::Success;
}
