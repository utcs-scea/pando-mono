// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_
#define PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_

#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/thread_local_storage.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
#include <pando-rt/memory/memory_guard.hpp>

namespace galois {

struct ELVertex {
  std::uint64_t id;
  constexpr operator std::uint64_t() {
    return id;
  }
};

struct ELEdge {
  std::uint64_t src;
  std::uint64_t dst;
  constexpr operator std::uint64_t() {
    return src;
  }
};

void loadELFilePerThread(
    galois::WaitGroup::HandleType wgh, pando::Array<char> filename, std::uint64_t segmentsPerThread,
    std::uint64_t numThreads, std::uint64_t threadID,
    galois::ThreadLocalVector<pando::Vector<ELEdge>> localReadEdges,
    galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename,
    std::uint64_t numVertices);

const char* elGetOne(const char* line, std::uint64_t& val);

template <typename EdgeFunc>
pando::Status elParse(const char* line, EdgeFunc efunc) {
  std::uint64_t src;
  std::uint64_t dst;
  line = elGetOne(line, src);
  line = elGetOne(line, dst);
  return efunc(src, dst);
}

pando::Status generateEdgesPerVirtualHost(pando::GlobalRef<pando::Vector<ELVertex>> vertices,
                                          std::uint64_t totalVertices, std::uint64_t vHostID,
                                          std::uint64_t numVHosts);

template <typename ReturnType, typename VertexType, typename EdgeType>
ReturnType initializeELDLCSR(pando::Array<char> filename, std::uint64_t numVertices,
                             std::uint64_t vHostsScaleFactor = 8) {
  galois::PerThreadVector<pando::Vector<ELEdge>> localEdges;
  PANDO_CHECK(localEdges.initialize());

  const std::uint64_t numThreads = localReadEdges.size() - pando::getPlaceDims().node.id;

  galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename;
  PANDO_CHECK(perThreadRename.initialize());

  for (auto hashRef : perThreadRename) {
    hashRef = galois::HashTable<std::uint64_t, std::uint64_t>{};
    PANDO_CHECK(fmap(hashRef, initialize, 0));
  }

  std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  const std::uint64_t numVHosts = hosts * vHostsScaleFactor;

  galois::WaitGroup wg;
  PANDO_CHECK(wg.initialize(numThreads));
  auto wgh = wg.getHandle();

  galois::DAccumulator<std::uint64_t> totVerts;
  PANDO_CHECK(totVerts.initialize());

  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread Start");
  for (std::uint64_t i = 0; i < numThreads; i++) {
    pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i % hosts)},
                                      pando::anyPod, pando::anyCore};
    PANDO_CHECK(pando::executeOn(place, &galois::loadELFilePerThread, wgh, filename, 1, numThreads,
                                 i, localReadEdges, perThreadRename, numVertices));
  }

  pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts;
  pando::LocalStorageGuard labeledEdgeCountsGuard(labeledEdgeCounts, 1);
  PANDO_CHECK(wg.wait());
  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread End");

#ifdef FREE
  auto freePerThreadRename =
      +[](galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>>
              perThreadRename) {
        for (galois::HashTable<std::uint64_t, std::uint64_t> hash : perThreadRename) {
          hash.deinitialize();
        }
      };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freePerThreadRename, perThreadRename));
  perThreadRename.deinitialize();
#endif

  PANDO_CHECK(galois::internal::buildEdgeCountToSend<ELEdge>(numVHosts, localReadEdges,
                                                             *labeledEdgeCounts));

  auto [v2PM, numEdges] = PANDO_EXPECT_CHECK(
      galois::internal::buildVirtualToPhysicalMapping(hosts, *labeledEdgeCounts));

#if FREE
  auto freeLabeledEdgeCounts =
      +[](pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledEdgeCounts) {
        labeledEdgeCounts.deinitialize();
      };
  PANDO_CHECK(pando::executeOn(
      pando::anyPlace, freeLabeledEdgeCounts,
      static_cast<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>>(*labeledEdgeCounts)));
#endif

  galois::HostIndexedMap<pando::Vector<ELVertex>> pHV{};
  PANDO_CHECK(pHV.initialize());

  /**
   * Make the vertices
   */
  struct GenerateVerticesState {
    std::uint64_t numVertices;
    pando::Array<std::uint64_t> v2PM;
  };

  auto generateVerticesPerHost =
      +[](GenerateVerticesState genVerts, pando::GlobalRef<pando::Vector<ELVertex>> vertices) {
        PANDO_CHECK(fmap(vertices, initialize, 0));
        const std::uint64_t host = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
        const std::uint64_t numVHosts = genVerts.v2PM.size();
        for (std::uint64_t i = 0; i < numVHosts; i++) {
          if (genVerts.v2PM[i] == host) {
            PANDO_CHECK(generateEdgesPerVirtualHost(vertices, genVerts.numVertices, i, numVHosts));
          }
        }
      };

  PANDO_CHECK(
      galois::doAll(GenerateVerticesState{numVertices, v2PM}, pHV, generateVerticesPerHost));

  auto [partEdges, renamePerHost] =
      internal::partitionEdgesParallely(pHV, std::move(localReadEdges), v2PM);

  using Graph = ReturnType;
  Graph graph;
  graph.template initializeAfterGather<galois::ELVertex, galois::ELEdge>(
      pHV, numVertices, partEdges, renamePerHost, numEdges,
      PANDO_EXPECT_CHECK(galois::copyToAllHosts(std::move(v2PM))));

#if FREE
  auto freeTheRest = +[](decltype(pHV) pHV, decltype(partEdges) partEdges,
                         decltype(renamePerHost) renamePerHost, decltype(numEdges) numEdges) {
    for (pando::Vector<ELVertex> vV : pHV) {
      vV.deinitialize();
    }
    pHV.deinitialize();
    for (pando::Vector<pando::Vector<ELEdge>> vVE : partEdges) {
      for (pando::Vector<ELEdge> vE : vVE) {
        vE.deinitialize();
      }
      vVE.deinitialize();
    }
    partEdges.deinitialize();
    renamePerHost.deinitialize();
    numEdges.deinitialize();
  };

  PANDO_CHECK(
      pando::executeOn(pando::anyPlace, freeTheRest, pHV, partEdges, renamePerHost, numEdges));
#endif

  wg.deinitialize();
  return graph;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_
