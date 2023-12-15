// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-lib-galois/import/schema.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

namespace galois::internal {

/**
 * @brief This is used to parallel insert Edges
 */
template <typename EdgeType>
[[nodiscard]] inline pando::Status insertLocalEdgesPerThread(
    pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef,
    pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> localEdges, EdgeType edge) {
  uint64_t result;

  if (fmap(hashRef, get, edge.src, result)) {
    pando::GlobalRef<pando::Vector<EdgeType>> vec = fmap(localEdges, get, result);
    return fmap(vec, pushBack, edge);
  } else {
    auto err = fmap(hashRef, put, edge.src, lift(localEdges, size));
    if (err != pando::Status::Success) {
      return err;
    }
    pando::Vector<EdgeType> v;
    err = v.initialize(1);
    if (err != pando::Status::Success) {
      return err;
    }
    v[0] = std::move(edge);
    return fmap(localEdges, pushBack, std::move(v));
  }
}

/**
 * @brief fills out the metadata for the VirtualToPhysical Host mapping.
 */
template <typename EdgeType>
void buildEdgeCountToSend(
    uint32_t numVirtualHosts, galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::GlobalRef<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts) {
  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> sumArray;
  PANDO_CHECK(sumArray.initialize(numVirtualHosts));

  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    galois::Pair<std::uint64_t, std::uint64_t> p{0, i};
    sumArray[i] = p;
  }

  galois::doAll(
      sumArray, localEdges,
      +[](pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> counts,
          pando::Vector<pando::Vector<EdgeType>> localEdges) {
        for (pando::Vector<EdgeType> v : localEdges) {
          assert(v.size() != 0);
          EdgeType e = v[0];
          pando::GlobalPtr<char> toAdd = static_cast<pando::GlobalPtr<char>>(
              static_cast<pando::GlobalPtr<void>>(&counts[e.src % counts.size()]));
          using UPair = galois::Pair<std::uint64_t, std::uint64_t>;
          toAdd += offsetof(UPair, first);
          pando::GlobalPtr<std::uint64_t> p = static_cast<pando::GlobalPtr<std::uint64_t>>(
              static_cast<pando::GlobalPtr<void>>(toAdd));
          pando::atomicFetchAdd(p, v.size(), std::memory_order_relaxed);
        }
      });

  labeledEdgeCounts = sumArray;
}

[[nodiscard]] pando::Status buildVirtualToPhysicalMapping(
    std::uint64_t numHosts,
    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledVirtualCounts,
    pando::GlobalPtr<pando::Array<std::uint64_t>> virtualToPhysicalMapping) {
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
  }

  for (auto it = labeledVirtualCounts.rbegin(); it != labeledVirtualCounts.rend(); it++) {
    galois::Pair<std::uint64_t, std::uint64_t> virtualPair = *it;
    // Find minimum
    std::sort(intermediateSort.begin(), intermediateSort.end());
    // Get Minimum
    galois::Pair<std::uint64_t, std::uint64_t> physicalPair = intermediateSort[0];
    // Store the id mappings
    vTPH[virtualPair.second] = physicalPair.second;
    // Update the count
    physicalPair.first += virtualPair.first;
    // Store back
    intermediateSort[0] = physicalPair;
  }

  intermediateSort.deinitialize();

  *virtualToPhysicalMapping = vTPH;
  return pando::Status::Success;
}

inline std::uint64_t getPhysical(std::uint64_t id,
                                 pando::Array<std::uint64_t> virtualToPhysicalMapping) {
  return virtualToPhysicalMapping[id % virtualToPhysicalMapping.size()];
}

/**
 * @brief PerHostVertex decomposition by partition
 *
 * TODO(AdityaAtulTewari) parallelize this
 */
template <typename VertexType>
[[nodiscard]] pando::Status perHostPartitionVertex(
    pando::Array<std::uint64_t> virtualToPhysicalMapping, pando::Vector<VertexType> vertices,
    pando::GlobalPtr<galois::PerHost<pando::Vector<VertexType>>> partitionedVertices) {
  pando::Status err;
  galois::PerHost<pando::Vector<VertexType>> partitioned;
  err = partitioned.initialize();
  if (err != pando::Status::Success) {
    return err;
  }

  std::uint64_t initSize = vertices.size() / lift(*partitionedVertices, size);
  for (pando::GlobalRef<pando::Vector<VertexType>> vec : partitioned) {
    err = fmap(vec, initialize, 0);
    if (err != pando::Status::Success) {
      return err;
    }
    err = fmap(vec, reserve, initSize);
    if (err != pando::Status::Success) {
      return err;
    }
  }

  for (VertexType vert : vertices) {
    err = fmap(partitioned.get(getPhysical(vert.id, virtualToPhysicalMapping)), pushBack, vert);
    if (err != pando::Status::Success) {
      return err;
    }
  }

  *partitionedVertices = partitioned;
  return pando::Status::Success;
}

/**
 * @brief Serially build the edgeLists
 */
template <typename EdgeType>
[[nodiscard]] pando::Status partitionEdgesSerially(
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::Array<std::uint64_t> virtualToPhysicalMapping,
    galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partitionedEdges) {
  galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
  auto err = renamePerHost.initialize();
  if (err != pando::Status::Success) {
    return err;
  }
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    err = hash.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    hashRef = hash;
  }

  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<pando::Vector<EdgeType>> threadLocalEdges = *localEdges.get(i);
    for (pando::Vector<EdgeType> vec : threadLocalEdges) {
      for (EdgeType edge : vec) {
        auto tgtHost = getPhysical(edge.src, virtualToPhysicalMapping);
        pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> edges =
            partitionedEdges.get(tgtHost);
        err = insertLocalEdgesPerThread(renamePerHost.get(tgtHost), edges, edge);
        if (err != pando::Status::Success) {
          return err;
        }
      }
    }
  }
  renamePerHost.deinitialize();
  return pando::Status::Success;
}

} // namespace galois::internal

#endif // PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
