// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_
#define PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_

#include <algorithm>
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
  bool operator<(const ELEdge& other) const {
    return (src == other.src) ? (dst < other.dst) : (src < other.src);
  }
};

void loadELFilePerThread(
    pando::Array<char> filename, std::uint64_t segmentsPerThread, std::uint64_t numThreads,
    std::uint64_t threadID, galois::ThreadLocalVector<pando::Vector<ELEdge>> localReadEdges,
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

pando::Vector<pando::Vector<ELEdge>> reduceLocalEdges(
    galois::ThreadLocalVector<pando::Vector<galois::ELEdge>> localEdges, uint64_t numVertices);

template <typename ReturnType, typename VertexType, typename EdgeType>
ReturnType initializeELDACSR(pando::Array<char> filename, std::uint64_t numVertices) {
  galois::ThreadLocalVector<pando::Vector<ELEdge>> localReadEdges;
  PANDO_CHECK(localReadEdges.initialize());

  const std::uint64_t numThreads = localReadEdges.size() - pando::getPlaceDims().node.id;

  galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename;
  PANDO_CHECK(perThreadRename.initialize());

  for (auto hashRef : perThreadRename) {
    hashRef = galois::HashTable<std::uint64_t, std::uint64_t>{};
    PANDO_CHECK(fmap(hashRef, initialize, 0));
  }

  galois::WaitGroup wg;
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();

  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread Start");
  auto elFileTuple = galois::make_tpl(filename, 1, localReadEdges, perThreadRename, numVertices);
  PANDO_CHECK(galois::doAllEvenlyPartition(
      wgh, elFileTuple, numThreads,
      +[](decltype(elFileTuple) elFileTuple, uint64_t i, uint64_t numThreads) {
        auto [filename, striping, localReadEdges, perThreadRename, numVertices] = elFileTuple;
        galois::loadELFilePerThread(filename, striping, numThreads, i, localReadEdges,
                                    perThreadRename, numVertices);
      }));
  PANDO_CHECK(wg.wait());
  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread End");

  pando::Vector<pando::Vector<ELEdge>> edgeList = reduceLocalEdges(localReadEdges, numVertices);

#ifdef FREE
  galois::WaitGroup freeWaiter;
  PANDO_CHECK(freeWaiter.initialize(0));
  auto freeWGH = freeWaiter.getHandle();
  galois::doAll(
      freeWGH, perThreadRename, +[](galois::HashTable<std::uint64_t, std::uint64_t> hash) {
        hash.deinitialize();
      });
  perThreadRename.deinitialize();
  freeWaiter.deinitialize();
#endif

  using Graph = ReturnType;
  Graph graph;
  PANDO_CHECK(graph.initialize(edgeList));

  for (uint64_t i = 0; i < numVertices; i++) {
    pando::Vector<ELEdge> ev = edgeList[i];
    ev.deinitialize();
    edgeList[i] = std::move(ev);
  }

  return graph;
}

pando::Status generateEdgesPerVirtualHost(pando::GlobalRef<pando::Vector<ELVertex>> vertices,
                                          std::uint64_t totalVertices, std::uint64_t vHostID,
                                          std::uint64_t numVHosts);

template <typename ReturnType, typename VertexType, typename EdgeType>
ReturnType initializeELDLCSR(pando::Array<char> filename, std::uint64_t numVertices,
                             std::uint64_t vHostsScaleFactor = 8) {
  galois::ThreadLocalVector<pando::Vector<ELEdge>> localReadEdges;
  PANDO_CHECK(localReadEdges.initialize());

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
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();

  galois::DAccumulator<std::uint64_t> totVerts;
  PANDO_CHECK(totVerts.initialize());

  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread Start");
  auto elFileTuple = galois::make_tpl(filename, 1, localReadEdges, perThreadRename, numVertices);
  PANDO_CHECK(galois::doAllEvenlyPartition(
      wgh, elFileTuple, numThreads,
      +[](decltype(elFileTuple) elFileTuple, uint64_t i, uint64_t numThreads) {
        auto [filename, striping, localReadEdges, perThreadRename, numVertices] = elFileTuple;
        galois::loadELFilePerThread(filename, striping, numThreads, i, localReadEdges,
                                    perThreadRename, numVertices);
      }));

  PANDO_CHECK(wg.wait());
  PANDO_MEM_STAT_NEW_KERNEL("loadELFilePerThread End");

#ifdef FREE
  galois::WaitGroup freeWaiter;
  PANDO_CHECK(freeWaiter.initialize(0));
  auto freeWGH = freeWaiter.getHandle();
  galois::doAll(
      freeWGH, perThreadRename, +[](galois::HashTable<std::uint64_t, std::uint64_t> hash) {
        hash.deinitialize();
      });
#endif

  auto labeledEdgeCounts =
      PANDO_EXPECT_CHECK(galois::internal::buildEdgeCountToSend<ELEdge>(numVHosts, localReadEdges));

  auto [v2PM, numEdges] =
      PANDO_EXPECT_CHECK(galois::internal::buildVirtualToPhysicalMapping(hosts, labeledEdgeCounts));

#if FREE
  auto freeLabeledEdgeCounts =
      +[](pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledEdgeCounts) {
        labeledEdgeCounts.deinitialize();
      };
  PANDO_CHECK(pando::executeOn(
      pando::anyPlace, freeLabeledEdgeCounts,
      static_cast<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>>(labeledEdgeCounts)));
  PANDO_CHECK(freeWaiter.wait());
  perThreadRename.deinitialize();
#endif
  auto hostLocalV2PM = PANDO_EXPECT_CHECK(galois::copyToAllHosts(std::move(v2PM)));

  galois::HostLocalStorage<pando::Vector<ELVertex>> pHV{};
  PANDO_CHECK(pHV.initialize());

  /**
   * Make the vertices
   */
  auto generateVerticesState = galois::make_tpl(numVertices, hostLocalV2PM);
  auto generateVerticesPerHost = +[](decltype(generateVerticesState) state,
                                     pando::GlobalRef<pando::Vector<ELVertex>> vertices) {
    auto [numVertices, hostLocalV2PM] = state;
    PANDO_CHECK(fmap(vertices, initialize, 0));
    const std::uint64_t host = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
    pando::Array<std::uint64_t> v2PM = hostLocalV2PM.getLocalRef();
    const std::uint64_t numVHosts = v2PM.size();
    for (std::uint64_t i = 0; i < numVHosts; i++) {
      if (v2PM[i] == host) {
        PANDO_CHECK(generateEdgesPerVirtualHost(vertices, numVertices, i, numVHosts));
      }
    }
  };

  PANDO_CHECK(galois::doAll(generateVerticesState, pHV, generateVerticesPerHost));

  auto [partEdges, renamePerHost] =
      internal::partitionEdgesParallely(pHV, std::move(localReadEdges), hostLocalV2PM);

  galois::doAll(
      partEdges, +[](pando::GlobalRef<pando::Vector<pando::Vector<ELEdge>>> edge_vectors) {
        pando::Vector<pando::Vector<ELEdge>> evs_tmp = edge_vectors;
        galois::doAll(
            evs_tmp, +[](pando::GlobalRef<pando::Vector<ELEdge>> src_ev) {
              pando::Vector<ELEdge> tmp = src_ev;
              std::sort(tmp.begin(), tmp.end());
              src_ev = tmp;
            });
        edge_vectors = evs_tmp;
      });

  using Graph = ReturnType;
  Graph graph;
  graph.template initializeAfterGather<galois::ELVertex, galois::ELEdge>(
      pHV, numVertices, partEdges, renamePerHost, numEdges, hostLocalV2PM);

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
  freeWaiter.deinitialize();
#endif
  wg.deinitialize();
  return graph;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_INGEST_RMAT_EL_HPP_
