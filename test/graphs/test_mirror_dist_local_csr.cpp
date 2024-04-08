// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

pando::Vector<pando::Vector<std::uint64_t>> generateFullyConnectedGraph(std::uint64_t SIZE) {
  pando::Vector<pando::Vector<std::uint64_t>> vec;
  EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
  for (pando::GlobalRef<pando::Vector<std::uint64_t>> edges : vec) {
    pando::Vector<std::uint64_t> inner;
    EXPECT_EQ(inner.initialize(0), pando::Status::Success);
    edges = inner;
  }

  galois::doAll(
      SIZE, vec, +[](std::uint64_t size, pando::GlobalRef<pando::Vector<std::uint64_t>> innerRef) {
        pando::Vector<std::uint64_t> inner = innerRef;
        for (std::uint64_t i = 0; i < size; i++) {
          EXPECT_EQ(inner.pushBack(i), pando::Status::Success);
        }
        innerRef = inner;
      });
  return vec;
}

template <typename T>
pando::Status deleteVectorVector(pando::Vector<pando::Vector<T>> vec) {
  auto err = galois::doAll(
      vec, +[](pando::GlobalRef<pando::Vector<T>> innerRef) {
        pando::Vector<std::uint64_t> inner = innerRef;
        inner.deinitialize();
        innerRef = inner;
      });
  vec.deinitialize();
  return err;
}

using Graph = galois::MirrorDistLocalCSR<std::uint64_t, std::uint64_t>;

TEST(MirrorDistLocalCSR, NumVertices) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);

  EXPECT_EQ(deleteVectorVector(vec), pando::Status::Success);
}

void getVerticesAndEdgesEL(const std::string& filename, std::uint64_t numVertices,
                           std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>& graph) {
  std::ifstream file(filename);
  std::uint64_t src, dst;
  while (file >> src && file >> dst) {
    if (src < numVertices && dst < numVertices) {
      graph[src].push_back(dst);
    }
  }
  for (std::uint64_t i = 0; i < numVertices; i++) {
    if (graph.find(i) == graph.end()) {
      graph[i] = std::vector<std::uint64_t>();
    }
  }
  file.close();
}

template <typename T>
bool isInVector(T element, pando::GlobalRef<pando::Vector<T>> vec) {
  for (std::uint64_t i = 0; i < lift(vec, size); i++) {
    T comp = fmap(vec, operator[], i);
    if (comp == element) {
      return true;
    }
  }
  return false;
}

class MirrorDLCSRInitEdgeList
    : public ::testing::TestWithParam<std::tuple<const char*, std::uint64_t>> {};

TEST_P(MirrorDLCSRInitEdgeList, MapExchange) {
  using ET = galois::ELEdge;
  using VT = galois::ELVertex;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;
  galois::HostLocalStorageHeap::HeapInit();

  const std::string elFile = std::get<0>(GetParam());
  const std::uint64_t numVertices = std::get<1>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  Graph graph =
      galois::initializeELDLCSR<Graph, galois::ELVertex, galois::ELEdge>(filename, numVertices);

  auto dims = pando::getPlaceDims();
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    pando::GlobalRef<pando::Array<Graph::MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap =
        graph.getLocalMirrorToRemoteMasterOrderedMap(nodeId);
    for (std::uint64_t i = 0ul; i < lift(localMirrorToRemoteMasterOrderedMap, size); i++) {
      Graph::MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, get, i);
      Graph::VertexTopologyID masterTopologyID = m.getMaster();
      Graph::VertexTokenID masterTokenID = graph.getTokenID(masterTopologyID);
      std::uint64_t physicalHost = graph.getPhysicalHostID(masterTokenID);
      pando::GlobalRef<pando::Vector<pando::Vector<Graph::MirrorToMasterMap>>>
          localMasterToRemoteMirrorMap = graph.getLocalMasterToRemoteMirrorMap(physicalHost);
      pando::GlobalRef<pando::Vector<Graph::MirrorToMasterMap>> mapVectorFromHost =
          fmap(localMasterToRemoteMirrorMap, get, nodeId);
      EXPECT_TRUE(isInVector<Graph::MirrorToMasterMap>(m, mapVectorFromHost));
    }
  }
  graph.deinitialize();
}

void TestFunc(galois::ELVertex mirror, pando::GlobalRef<galois::ELVertex> master) {
  fmapVoid(master, set, mirror.get() + 1);
}

TEST_P(MirrorDLCSRInitEdgeList, Reduce) {
  using ET = galois::ELEdge;
  using VT = galois::ELVertex;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;
  galois::HostLocalStorageHeap::HeapInit();

  const std::string elFile = std::get<0>(GetParam());
  const std::uint64_t numVertices = std::get<1>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  Graph graph =
      galois::initializeELDLCSR<Graph, galois::ELVertex, galois::ELEdge>(filename, numVertices);

  auto dims = pando::getPlaceDims();

  galois::GlobalBarrier barrier;
  EXPECT_EQ(barrier.initialize(dims.node.id), pando::Status::Success);

  auto func = +[](galois::GlobalBarrier barrier,
                  galois::HostLocalStorage<pando::Array<bool>> mirrorBitSets) {
    pando::GlobalRef<pando::Array<bool>> mirrorBitSet =
        mirrorBitSets[pando::getCurrentPlace().node.id];
    fmapVoid(mirrorBitSet, fill, true);
    barrier.done();
  };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, barrier, graph.getMirrorBitSets()),
        pando::Status::Success);
  }
  EXPECT_EQ(barrier.wait(), pando::Status::Success);

  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    pando::GlobalRef<pando::Array<bool>> mirrorBitSet = graph.getMirrorBitSet(nodeId);
    for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
      EXPECT_EQ(fmap(mirrorBitSet, get, i), true);
    }
  }

  graph.reduce(TestFunc);

  std::uint64_t sumMaster = 0;
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    pando::GlobalRef<pando::Array<bool>> masterBitSet = graph.getMasterBitSet(nodeId);
    for (std::uint64_t i = 0ul; i < lift(masterBitSet, size); i++) {
      if (fmap(masterBitSet, get, i) == true) {
        sumMaster += 1;
      }
    }
  }
  EXPECT_EQ(sumMaster, graph.sizeMirrors());

  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    pando::GlobalRef<pando::Array<bool>> mirrorBitSet = graph.getMirrorBitSet(nodeId);
    pando::GlobalRef<pando::Array<Graph::MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap =
        graph.getLocalMirrorToRemoteMasterOrderedMap(nodeId);
    for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
      Graph::MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, get, i);
      Graph::VertexTopologyID mirrorTopologyID = m.getMirror();
      Graph::VertexTopologyID masterTopologyID = m.getMaster();
      Graph::VertexTokenID masterTokenID = graph.getTokenID(masterTopologyID);
      std::uint64_t physicalHost = graph.getPhysicalHostID(masterTokenID);

      pando::GlobalRef<pando::Array<bool>> masterBitSet = graph.getMasterBitSet(physicalHost);
      std::uint64_t index = graph.getIndex(masterTopologyID, graph.getMasterRange(physicalHost));
      EXPECT_EQ(fmap(masterBitSet, get, index), true);

      Graph::VertexData mirrorData = graph.getData(mirrorTopologyID);
      Graph::VertexData masterData = graph.getData(masterTopologyID);
      EXPECT_EQ(masterData.get(), mirrorData.get() + 1);
    }
  }

  graph.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, MirrorDLCSRInitEdgeList,
    ::testing::Values(std::make_tuple("/pando/graphs/simple.el", 10),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el",
                                      1024)));

INSTANTIATE_TEST_SUITE_P(
    DISABLED_BigFiles, MirrorDLCSRInitEdgeList,
    ::testing::Values(
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el", 2048),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el", 4096),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el", 8192),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el", 16384)));
