// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include <pando-lib-galois/containers/array.hpp>
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/host_indexed_map.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-lib-galois/import/schema.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
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
    PANDO_CHECK_RETURN(fmap(hashRef, put, edge.src, lift(localEdges, size)));
    pando::Vector<EdgeType> v;
    PANDO_CHECK_RETURN(v.initialize(1));
    v[0] = std::move(edge);
    return fmap(localEdges, pushBack, std::move(v));
  }
}

/**
 * @brief fills out the metadata for the VirtualToPhysical Host mapping.
 */
template <typename EdgeType>
[[nodiscard]] pando::Status buildEdgeCountToSend(
    std::uint64_t numVirtualHosts, galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::GlobalRef<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts) {
  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> sumArray;
  PANDO_CHECK_RETURN(sumArray.initialize(numVirtualHosts));

  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    galois::Pair<std::uint64_t, std::uint64_t> p{0, i};
    sumArray[i] = p;
  }

  PANDO_CHECK_RETURN(galois::doAll(
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
      }));
  labeledEdgeCounts = sumArray;
  return pando::Status::Success;
}

/**
 * @brief fills out the metadata for the VirtualToPhysical Host mapping.
 */
template <typename EdgeType>
void buildEdgeCountToSend(
    std::uint64_t numVirtualHosts, galois::PerThreadVector<EdgeType> localEdges,
    pando::GlobalRef<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts) {
  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> sumArray;
  PANDO_CHECK(sumArray.initialize(numVirtualHosts));

  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    galois::Pair<std::uint64_t, std::uint64_t> p{0, i};
    sumArray[i] = p;
  }
  galois::WaitGroup wg{};
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();

  using UPair = galois::Pair<std::uint64_t, std::uint64_t>;
  const uint64_t pairOffset = offsetof(UPair, first);

  auto state = galois::make_tpl(wgh, sumArray);
  PANDO_CHECK(galois::doAll(
      wgh, state, localEdges, +[](decltype(state) state, pando::Vector<EdgeType> localEdges) {
        auto [wgh, sumArray] = state;
        PANDO_CHECK(galois::doAll(
            wgh, sumArray, localEdges, +[](decltype(sumArray) counts, EdgeType localEdge) {
              pando::GlobalPtr<char> toAdd = static_cast<pando::GlobalPtr<char>>(
                  static_cast<pando::GlobalPtr<void>>(&counts[localEdge.src % counts.size()]));
              toAdd += pairOffset;
              pando::GlobalPtr<std::uint64_t> p = static_cast<pando::GlobalPtr<std::uint64_t>>(
                  static_cast<pando::GlobalPtr<void>>(toAdd));
              pando::atomicFetchAdd(p, 1, std::memory_order_relaxed);
            }));
      }));
  PANDO_CHECK(wg.wait());
  labeledEdgeCounts = sumArray;
}

[[nodiscard]] pando::Expected<
    galois::Pair<pando::Array<std::uint64_t>, galois::HostIndexedMap<std::uint64_t>>>
buildVirtualToPhysicalMapping(
    std::uint64_t numHosts,
    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledVirtualCounts);

inline std::uint64_t getPhysical(std::uint64_t id,
                                 pando::Array<std::uint64_t> virtualToPhysicalMapping) {
  return virtualToPhysicalMapping[id % virtualToPhysicalMapping.size()];
}

template <typename A>
uint64_t transmute(A p) {
  return p;
}
template <typename A, typename B>
uint64_t scan_op(A p, B l) {
  return p + l;
}
template <typename B>
uint64_t combiner(B f, B s) {
  return f + s;
}

/**
 * @brief Consumes localEdges, and references a partitionMap to produced partitioned Edges grouped
 * by Vertex, along with a rename set of Vertices
 */
template <typename EdgeType, typename VertexType>
[[nodiscard]] Pair<HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>>,
                   HostIndexedMap<HashTable<std::uint64_t, std::uint64_t>>>
partitionEdgesParallely(HostIndexedMap<pando::Vector<VertexType>> partitionedVertices,
                        PerThreadVector<pando::Vector<EdgeType>>&& localEdges,
                        pando::Array<std::uint64_t> v2PM) {
  HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> partEdges{};
  PANDO_CHECK(partEdges.initialize());

  HostIndexedMap<HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
  PANDO_CHECK(renamePerHost.initialize());

  // Initialize hashmap
  uint64_t i = 0;
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    PANDO_CHECK(hash.initialize(0));
    hashRef = hash;
    i++;
  }
  // Insert into hashmap
  // ToDo: Parallelize this (Divija)

  PANDO_CHECK(galois::doAll(
      partitionedVertices, renamePerHost,
      +[](HostIndexedMap<pando::Vector<VertexType>> partitionedVertices,
          pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef) {
        uint64_t idx = 0;
        const std::uint64_t hostID = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
        pando::Vector<VertexType> partVert = partitionedVertices.get(hostID);
        pando::Vector<EdgeType> v;
        PANDO_CHECK(fmap(v, initialize, 0));
        for (VertexType vrtx : partVert) {
          PANDO_CHECK(fmap(hashRef, put, vrtx.id, idx));
          idx++;
        }
      }));

  i = 0;
  for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vvec : partEdges) {
    PANDO_CHECK(fmap(vvec, initialize, lift(renamePerHost.get(i), size)));
    pando::Vector<pando::Vector<EdgeType>> vec = vvec;
    for (pando::GlobalRef<pando::Vector<EdgeType>> v : vec) {
      PANDO_CHECK(fmap(v, initialize, 0));
    }
    i++;
  }
  std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  DistArray<HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>>> perThreadEdges;
  PANDO_CHECK(perThreadEdges.initialize(localEdges.size()));
  for (uint64_t i = 0; i < localEdges.size(); i++) {
    PANDO_CHECK(lift(perThreadEdges[i], initialize));
    HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> pVec = perThreadEdges[i];
    for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vec : pVec) {
      PANDO_CHECK(fmap(vec, initialize, 0));
    }
  }

  // Compute the number of edges per host per thread
  HostIndexedMap<galois::Array<uint64_t>> numEdgesPerHostPerThread{};
  HostIndexedMap<galois::Array<uint64_t>> prefixArrayPerHostPerThread{};
  PANDO_CHECK(numEdgesPerHostPerThread.initialize());
  PANDO_CHECK(prefixArrayPerHostPerThread.initialize());
  for (std::uint64_t i = 0; i < numHosts; i++) {
    PANDO_CHECK(fmap(prefixArrayPerHostPerThread.get(i), initialize, lift(localEdges, size)));
    PANDO_CHECK(fmap(numEdgesPerHostPerThread.get(i), initialize, lift(localEdges, size)));
  }

  auto state = galois::make_tpl(perThreadEdges, localEdges, v2PM, numEdgesPerHostPerThread);
  galois::doAllEvenlyPartition(
      state, lift(localEdges, size), +[](decltype(state) state, uint64_t tid, uint64_t) {
        auto [perThreadEdges, localEdgesPTV, V2PMap, prefixArr] = state;
        pando::GlobalPtr<pando::Vector<pando::Vector<EdgeType>>> localEdgesPtr =
            localEdgesPTV.get(tid);
        pando::Vector<pando::Vector<EdgeType>> localEdges = *localEdgesPtr;
        HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> edgeVec = perThreadEdges[tid];
        for (pando::Vector<EdgeType> vec : localEdges) {
          EdgeType e = vec[0];
          uint64_t hostID = V2PMap[e.src % V2PMap.size()];
          PANDO_CHECK(fmap(edgeVec.get(hostID), pushBack, vec));
        }
        std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
        for (uint64_t i = 0; i < numHosts; i++) {
          galois::Array<uint64_t> arr = prefixArr.get(i);
          *(arr.begin() + tid) = lift(edgeVec.get(i), size);
        }
      });

  // Compute prefix sum
  using SRC = galois::Array<uint64_t>;
  using DST = galois::Array<uint64_t>;
  using SRC_Val = uint64_t;
  using DST_Val = uint64_t;
  for (uint64_t i = 0; i < numHosts; i++) {
    galois::Array<uint64_t> arr = numEdgesPerHostPerThread.get(i);
    galois::Array<uint64_t> prefixArr = prefixArrayPerHostPerThread.get(i);
    galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, galois::internal::transmute<uint64_t>,
                      galois::internal::scan_op<SRC_Val, DST_Val>,
                      galois::internal::combiner<DST_Val>, galois::Array>
        prefixSum(arr, prefixArr);
    PANDO_CHECK(prefixSum.initialize());
    prefixSum.computePrefixSum(lift(localEdges, size));
  }
  HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> pHVEdge{};
  PANDO_CHECK(pHVEdge.initialize());
  for (uint64_t i = 0; i < numHosts; i++) {
    galois::Array<uint64_t> prefixArr = prefixArrayPerHostPerThread.get(i);
    PANDO_CHECK(fmap(pHVEdge.get(i), initialize, prefixArr[lift(localEdges, size) - 1]));
  }
  // Parallel insert
  auto PHState = galois::make_tpl(pHVEdge, prefixArrayPerHostPerThread, perThreadEdges);
  galois::doAllEvenlyPartition(
      PHState, lift(localEdges, size), +[](decltype(PHState) PHState, uint64_t threadID, uint64_t) {
        auto [phVec, prefixArrPHPT, newEdge] = PHState;
        std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
        for (uint64_t i = 0; i < numHosts; i++) {
          galois::Array<uint64_t> prefixArr = prefixArrPHPT.get(i);
          HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> PTVec = newEdge[threadID];
          pando::Vector<pando::Vector<EdgeType>> vert = PTVec.get(i);
          uint64_t start;
          if (threadID)
            start = prefixArr[threadID - 1];
          else
            start = 0;
          uint64_t end = prefixArr[threadID];
          uint64_t idx = 0;
          pando::Vector<pando::Vector<EdgeType>> vec = phVec.get(i);
          for (auto it = vec.begin() + start; it != vec.begin() + end; it++) {
            *it = vert.get(idx);
            idx++;
          }
        }
      });

  auto PHState1 = galois::make_tpl(partEdges, renamePerHost, pHVEdge);
  PANDO_CHECK(galois::doAllEvenlyPartition(
      PHState1, numHosts, +[](decltype(PHState1) PHState1, uint64_t host_id, uint64_t) {
        auto [partEdges, renamePH, pHVEdge] = PHState1;
        pando::Vector<pando::Vector<EdgeType>> vec;
        pando::Vector<pando::Vector<EdgeType>> exchangedVec = pHVEdge.get(host_id);
        galois::HashTable<std::uint64_t, std::uint64_t> hashMap = renamePH.get(host_id);
        uint64_t result;
        for (uint64_t j = 0; j < lift(exchangedVec, size); j++) {
          pando::GlobalRef<pando::Vector<EdgeType>> v = exchangedVec.get(j);
          pando::Vector<EdgeType> v1 = v;
          EdgeType e = v1[0];
          bool ret = fmap(hashMap, get, e.src, result);
          if (!ret) {
            return pando::Status::Error;
          }
          pando::GlobalRef<pando::Vector<EdgeType>> edgeVec =
              fmap(partEdges.get(host_id), get, result);
          PANDO_CHECK(fmap(edgeVec, append, &v));
        }
        return pando::Status::Success;
      }));
  return galois::make_tpl(partEdges, renamePerHost);
}

/**
 * @brief Serially build the edgeLists
 */
template <typename EdgeType>
[[nodiscard]] pando::Status partitionEdgesSerially(
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::Array<std::uint64_t> virtualToPhysicalMapping,
    galois::HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> partitionedEdges,
    galois::HostIndexedMap<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost) {
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    PANDO_CHECK_RETURN(hash.initialize(0));
    hashRef = hash;
  }
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<pando::Vector<EdgeType>> threadLocalEdges = *localEdges.get(i);
    for (pando::Vector<EdgeType> vec : threadLocalEdges) {
      for (EdgeType edge : vec) {
        auto tgtHost = getPhysical(edge.src, virtualToPhysicalMapping);
        pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> edges =
            partitionedEdges.get(tgtHost);
        PANDO_CHECK_RETURN(insertLocalEdgesPerThread(renamePerHost.get(tgtHost), edges, edge));
      }
    }
  }
  return pando::Status::Success;
}

/**
 * @brief Serially build the edgeLists
 */
template <typename EdgeType>
[[nodiscard]] pando::Status partitionEdgesSerially(
    galois::PerThreadVector<EdgeType> localEdges,
    pando::Array<std::uint64_t> virtualToPhysicalMapping,
    galois::HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> partitionedEdges,
    galois::HostIndexedMap<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost) {
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    PANDO_CHECK_RETURN(hash.initialize(0));
    hashRef = hash;
  }
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<EdgeType> threadLocalEdges = *localEdges.get(i);
    for (EdgeType edge : threadLocalEdges) {
      auto tgtHost = getPhysical(edge.src, virtualToPhysicalMapping);
      pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> edges =
          partitionedEdges.get(tgtHost);
      PANDO_CHECK_RETURN(insertLocalEdgesPerThread(renamePerHost.get(tgtHost), edges, edge));
    }
  }
  return pando::Status::Success;
}

template <typename VertexType>
[[nodiscard]] galois::HostIndexedMap<pando::Vector<VertexType>> partitionVerticesParallel(
    galois::PerThreadVector<VertexType> localVertices, pando::Array<std::uint64_t> v2PM) {
  DistArray<HostIndexedMap<pando::Vector<VertexType>>> perThreadVerticesPartition;
  PANDO_CHECK(perThreadVerticesPartition.initialize(localVertices.size()));
  std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  for (uint64_t i = 0; i < localVertices.size(); i++) {
    PANDO_CHECK(lift(perThreadVerticesPartition[i], initialize));
    HostIndexedMap<pando::Vector<VertexType>> pVec = perThreadVerticesPartition[i];
    for (uint64_t j = 0; j < numHosts; j++) {
      PANDO_CHECK(fmap(pVec.get(j), initialize, 0));
    }
  }

  HostIndexedMap<galois::Array<std::uint64_t>> numVerticesPerHostPerThread{};
  HostIndexedMap<galois::Array<std::uint64_t>> prefixArrPerHostPerThread{};
  PANDO_CHECK(numVerticesPerHostPerThread.initialize());
  PANDO_CHECK(prefixArrPerHostPerThread.initialize());
  for (std::uint64_t i = 0; i < numHosts; i++) {
    PANDO_CHECK(fmap(numVerticesPerHostPerThread.get(i), initialize, lift(localVertices, size)));
    PANDO_CHECK(fmap(prefixArrPerHostPerThread.get(i), initialize, lift(localVertices, size)));
  }

  auto newVec =
      make_tpl(perThreadVerticesPartition, localVertices, v2PM, numVerticesPerHostPerThread);
  galois::doAllEvenlyPartition(
      newVec, lift(localVertices, size), +[](decltype(newVec) newVec, uint64_t tid, uint64_t) {
        auto [perThreadVerticesPT, localVerticesVec, v2PMap, prefixArr] = newVec;
        pando::GlobalPtr<pando::Vector<VertexType>> localVerticesPtr = localVerticesVec.get(tid);
        pando::Vector<VertexType> localVertices = *localVerticesPtr;
        HostIndexedMap<pando::Vector<VertexType>> vertVec = perThreadVerticesPT[tid];
        for (VertexType v : localVertices) {
          uint64_t hostID = v2PMap[v.id % v2PMap.size()];
          PANDO_CHECK(fmap(vertVec.get(hostID), pushBack, v));
        }
        std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
        for (uint64_t i = 0; i < numHosts; i++) {
          galois::Array<uint64_t> arr = prefixArr.get(i);
          *(arr.begin() + tid) = lift(vertVec.get(i), size);
        }
      });

  // Compute prefix sum
  using SRC = galois::Array<uint64_t>;
  using DST = galois::Array<uint64_t>;
  using SRC_Val = uint64_t;
  using DST_Val = uint64_t;

  for (uint64_t i = 0; i < numHosts; i++) {
    galois::Array<uint64_t> arr = numVerticesPerHostPerThread.get(i);
    galois::Array<uint64_t> prefixArr = prefixArrPerHostPerThread.get(i);
    galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, galois::internal::transmute<uint64_t>,
                      galois::internal::scan_op<SRC_Val, DST_Val>,
                      galois::internal::combiner<DST_Val>, galois::Array>
        prefixSum(arr, prefixArr);
    PANDO_CHECK(prefixSum.initialize());
    prefixSum.computePrefixSum(lift(localVertices, size));
  }

  galois::HostIndexedMap<pando::Vector<VertexType>> pHV{};
  PANDO_CHECK(pHV.initialize());

  for (uint64_t i = 0; i < numHosts; i++) {
    galois::Array<uint64_t> prefixArr = prefixArrPerHostPerThread.get(i);
    PANDO_CHECK(fmap(pHV.get(i), initialize, prefixArr[lift(localVertices, size) - 1]));
  }

  auto phVec = make_tpl(pHV, prefixArrPerHostPerThread, perThreadVerticesPartition);
  galois::doAllEvenlyPartition(
      phVec, lift(localVertices, size), +[](decltype(phVec) phVec, uint64_t threadID, uint64_t) {
        auto [pHV, prefixArrPerHost, PHVertex] = phVec;
        std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
        for (uint64_t i = 0; i < numHosts; i++) {
          galois::Array<uint64_t> prefixArr = prefixArrPerHost.get(i);
          HostIndexedMap<pando::Vector<VertexType>> PHvec = PHVertex[threadID];
          pando::Vector<VertexType> vert = PHvec.get(i);
          uint64_t start;
          if (threadID)
            start = prefixArr[threadID - 1];
          else
            start = 0;
          uint64_t end = prefixArr[threadID];
          uint64_t idx = 0;
          pando::Vector<VertexType> vec = pHV.get(i);
          for (auto it = vec.begin() + start; it != vec.begin() + end; it++) {
            *it = vert.get(idx);
            idx++;
          }
        }
      });
  return pHV;
}

/**
 * @brief Consumes localEdges, and references a partitionMap to produced partitioned Edges grouped
 * by Vertex, along with a rename set of Vertices
 */
template <typename EdgeType>
[[nodiscard]] Pair<HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>>,
                   HostIndexedMap<HashTable<std::uint64_t, std::uint64_t>>>
partitionEdgesPerHost(PerThreadVector<pando::Vector<EdgeType>>&& localEdges,
                      pando::Array<std::uint64_t> v2PM) {
  HostIndexedMap<pando::Vector<pando::Vector<EdgeType>>> partEdges{};
  PANDO_CHECK(partEdges.initialize());

  for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vvec : partEdges) {
    PANDO_CHECK(fmap(vvec, initialize, 0));
  }

  HostIndexedMap<HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
  PANDO_CHECK(renamePerHost.initialize());

  PANDO_CHECK(galois::internal::partitionEdgesSerially(localEdges, v2PM, partEdges, renamePerHost));

#if FREE
  auto freeLocalEdges = +[](PerThreadVector<pando::Vector<EdgeType>> localEdges) {
    for (pando::Vector<pando::Vector<EdgeType>> vVE : localEdges) {
      for (pando::Vector<EdgeType> vE : vVE) {
        vE.deinitialize();
      }
    }
    localEdges.deinitialize();
  };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freeLocalEdges, localEdges));
#endif
  return galois::make_tpl(partEdges, renamePerHost);
}

uint64_t inline getFileReadOffset(galois::ifstream& file, uint64_t segment, uint64_t numSegments) {
  uint64_t fileSize = file.size();
  if (segment == 0) {
    return 0;
  }
  if (segment >= numSegments) {
    return fileSize;
  }

  uint64_t bytesPerSegment = fileSize / numSegments;
  uint64_t offset = segment * bytesPerSegment;
  pando::Vector<char> line;
  PANDO_CHECK(line.initialize(0));

  // check for partial line at start
  file.seekg(offset - 1);
  file.getline(line, '\n');
  // if not at start of a line, discard partial line
  if (!line.empty())
    offset += line.size();
  line.deinitialize();
  return offset;
}

/*
 * Load graph info from the file.
 *
 * @param filename loaded file for the graph
 * @param segmentsPerHost the number of file segments each host will load.
 * If value is 1, no file striping is performed. The file is striped into
 * (segementsPerHost * numHosts) segments.
 *
 * @details File striping is used to randomize the order of nodes/edges
 * loaded from the graph. WMD dataset csv typically grouped nodes/edges by its types,
 * which will produce an imbalanced graph if you break the file evenly among hosts.
 * So file striping make each host be able to load multiple segments in different positions of the
 * file, which produced a more balanced graph.
 *
 * @note per thread method
 */
template <typename ParseFunc>
pando::Status loadGraphFilePerThread(pando::Array<char> filename, uint64_t segmentsPerThread,
                                     std::uint64_t numThreads, std::uint64_t threadID,
                                     ParseFunc parseFunc) {
  galois::ifstream graphFile;
  PANDO_CHECK_RETURN(graphFile.open(filename));
  uint64_t numSegments = numThreads * segmentsPerThread;

  // init per thread data struct

  // for each host N, it will read segment like:
  // N, N + numHosts, N + numHosts * 2, ..., N + numHosts * (segmentsPerHost - 1)
  for (uint64_t cur = 0; cur < segmentsPerThread; cur++) {
    uint64_t segmentID = (uint64_t)threadID + cur * numThreads;
    uint64_t start = getFileReadOffset(graphFile, segmentID, numSegments);
    uint64_t end = getFileReadOffset(graphFile, segmentID + 1, numSegments);
    if (start == end) {
      continue;
    }
    graphFile.seekg(start);

    // load segment into memory
    uint64_t segmentLength = end - start;
    char* segmentBuffer = new char[segmentLength];
    graphFile.read(segmentBuffer, segmentLength);

    // A parallel loop that parse the segment
    // task 1: get token to global id mapping
    // task 2: get token to edges mapping
    char* currentLine = segmentBuffer;
    char* endLine = currentLine + segmentLength;

    while (currentLine < endLine) {
      assert(std::strchr(currentLine, '\n'));
      char* nextLine = std::strchr(currentLine, '\n') + 1;

      // skip comments
      if (currentLine[0] == '#') {
        currentLine = nextLine;
        continue;
      }

      PANDO_CHECK_RETURN(parseFunc(currentLine));
      currentLine = nextLine;
    }
    delete[] segmentBuffer;
  }
  graphFile.close();
  return pando::Status::Success;
}

/*
 * Load vertex info from a parser
 */
template <typename VertexType>
void loadVertexFilePerThread(pando::NotificationHandle done,
                             galois::VertexParser<VertexType> parser, uint64_t segmentsPerThread,
                             std::uint64_t numThreads, std::uint64_t threadID,
                             galois::PerThreadVector<VertexType> localVertices) {
  pando::GlobalRef<pando::Vector<VertexType>> localVert = localVertices.getThreadVector();
  auto parseLine = [&parser, &localVert](const char* currentLine) {
    if (currentLine[0] != parser.comment) {
      PANDO_CHECK_RETURN(fmap(localVert, pushBack, parser.parser(currentLine)));
    }
    return pando::Status::Success;
  };
  PANDO_CHECK(
      loadGraphFilePerThread(parser.filename, segmentsPerThread, numThreads, threadID, parseLine));
  done.notify();
}

template <typename EdgeType>
void loadEdgeFilePerThread(
    pando::NotificationHandle done, galois::EdgeParser<EdgeType> parser, uint64_t segmentsPerThread,
    std::uint64_t numThreads, std::uint64_t threadID,
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    galois::DistArray<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename) {
  auto hartID = localEdges.getLocalVectorID();
  auto localEdgeVec = localEdges.getThreadVector();
  auto hashRef = perThreadRename[hartID];

  auto parseLine = [&parser, &localEdgeVec, &hashRef](const char* currentLine) {
    if (currentLine[0] != parser.comment) {
      ParsedEdges<EdgeType> parsed = parser.parser(currentLine);
      if (parsed.isEdge) {
        PANDO_CHECK_RETURN(insertLocalEdgesPerThread(hashRef, localEdgeVec, parsed.edge1));
        if (parsed.has2Edges) {
          PANDO_CHECK_RETURN(insertLocalEdgesPerThread(hashRef, localEdgeVec, parsed.edge2));
        }
      }
    }
    return pando::Status::Success;
  };
  done.notify();
}

template <typename EdgeType>
struct ImportState {
  ImportState() = default;
  ImportState(galois::EdgeParser<EdgeType> parser_, galois::PerThreadVector<EdgeType> localEdges_)
      : parser(parser_), localEdges(localEdges_) {}

  galois::EdgeParser<EdgeType> parser;
  galois::PerThreadVector<EdgeType> localEdges;
};

template <typename EdgeType>
void loadGraphFile(ImportState<EdgeType>& state, uint64_t segmentID, uint64_t numSegments) {
  auto parseLine = [&state](const char* currentLine) {
    if (currentLine[0] == state.parser.comment) {
      return pando::Status::Success;
    }
    ParsedEdges<EdgeType> parsed = state.parser.parser(currentLine);
    if (parsed.isEdge) {
      PANDO_CHECK_RETURN(state.localEdges.pushBack(parsed.edge1));
      if (parsed.has2Edges) {
        PANDO_CHECK_RETURN(state.localEdges.pushBack(parsed.edge2));
      }
    }
    return pando::Status::Success;
  };
  PANDO_CHECK(loadGraphFilePerThread(state.parser.filename, 1, numSegments, segmentID, parseLine));
}

} // namespace galois::internal

#endif // PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
