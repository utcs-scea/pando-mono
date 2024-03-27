// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/wmd_graph_importer.hpp>

#include <pando-lib-galois/containers/host_indexed_map.hpp>

[[nodiscard]] pando::Expected<
    galois::Pair<pando::Array<std::uint64_t>, galois::HostIndexedMap<std::uint64_t>>>
galois::internal::buildVirtualToPhysicalMapping(
    std::uint64_t numHosts,
    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledVirtualCounts) {
  std::sort(labeledVirtualCounts.begin(), labeledVirtualCounts.end());

  pando::Array<std::uint64_t> vTPH;
  PANDO_CHECK_RETURN(vTPH.initialize(labeledVirtualCounts.size()));

  galois::HostIndexedMap<std::uint64_t> numEdges{};
  PANDO_CHECK_RETURN(numEdges.initialize());

  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> intermediateSort;
  PANDO_CHECK_RETURN(intermediateSort.initialize(numHosts));

  for (std::uint64_t i = 0; i < numHosts; i++) {
    intermediateSort[i] =
        galois::Pair<std::uint64_t, std::uint64_t>{static_cast<std::uint64_t>(0), i};
    numEdges.get(0) = 0;
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
    numEdges.get(physicalPair.second) = physicalPair.first;
    // Store back
    intermediateSort[0] = physicalPair;
  }

  intermediateSort.deinitialize();

  return galois::make_tpl(vTPH, numEdges);
}
