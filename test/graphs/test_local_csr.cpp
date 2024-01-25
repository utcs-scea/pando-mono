// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/graphs/local_csr.hpp>
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

using Graph = galois::LCSR<std::uint64_t, std::uint64_t>;

TEST(LCSR, NumVertices) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  auto err = deleteVectorVector<std::uint64_t>(vec);
  EXPECT_EQ(err, pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);
  graph.deinitialize();
}

TEST(LCSR, Locality) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  EXPECT_EQ(deleteVectorVector<std::uint64_t>(vec), pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);
  for (pando::GlobalRef<galois::Vertex> vert : graph.vertices()) {
    EXPECT_TRUE(graph.isLocal(vert));
    EXPECT_TRUE(graph.isOwned(vert));
    EXPECT_TRUE(pando::isSubsetOf(pando::getCurrentPlace(), graph.getLocalityVertex(vert)));
  }
  graph.deinitialize();
}

TEST(LCSR, VertexData) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  EXPECT_EQ(deleteVectorVector<std::uint64_t>(vec), pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);
  std::uint64_t i = 0;
  for (pando::GlobalRef<galois::Vertex> vert : graph.vertices()) {
    graph.setData(vert, i);
    i++;
  }

  i = 0;
  for (pando::GlobalRef<std::uint64_t> vdata : graph.vertexDataRange()) {
    EXPECT_EQ(vdata, i);
    i++;
  }
  graph.deinitialize();
}

TEST(LCSR, EdgeData) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  EXPECT_EQ(deleteVectorVector<std::uint64_t>(vec), pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);

  for (typename galois::graph_traits<Graph>::VertexTopologyID vert : graph.vertices()) {
    std::uint64_t j = 0;
    for (typename galois::graph_traits<Graph>::EdgeHandle eh : Graph::makeEdgeRange(vert)) {
      graph.setEdgeData(eh, j);
      graph.setEdgeData(vert, j, j);
      j++;
    }
  }

  for (typename galois::graph_traits<Graph>::VertexTopologyID vert : graph.vertices()) {
    std::uint64_t j = 0;
    for (typename galois::graph_traits<Graph>::EdgeHandle eh : Graph::makeEdgeRange(vert)) {
      EXPECT_EQ(graph.getEdgeData(eh), j);
      EXPECT_EQ(graph.getEdgeData(vert, j), j);
      j++;
    }
  }
  graph.deinitialize();
}

TEST(LCSR, DataRange) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  EXPECT_EQ(deleteVectorVector<std::uint64_t>(vec), pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);

  std::uint64_t i = 0;
  for (pando::GlobalRef<typename galois::graph_traits<Graph>::VertexData> vert :
       graph.vertexDataRange()) {
    vert = i;
    i++;
  }

  for (typename galois::graph_traits<Graph>::VertexTopologyID vert : graph.vertices()) {
    std::uint64_t j = 0;
    for (pando::GlobalRef<typename galois::graph_traits<Graph>::EdgeData> edata :
         graph.edgeDataRange(vert)) {
      edata = j;
      j++;
    }
  }

  i = 0;
  for (pando::GlobalRef<typename galois::graph_traits<Graph>::VertexData> vert :
       graph.vertexDataRange()) {
    EXPECT_EQ(vert, i);
    i++;
  }

  for (typename galois::graph_traits<Graph>::VertexTopologyID vert : graph.vertices()) {
    std::uint64_t j = 0;
    for (typename galois::graph_traits<Graph>::EdgeData edata : graph.edgeDataRange(vert)) {
      EXPECT_EQ(edata, j);
      j++;
    }
  }

  graph.deinitialize();
}

TEST(LCSR, VertexIndex) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  EXPECT_EQ(graph.initialize(vec), pando::Status::Success);
  auto err = deleteVectorVector<std::uint64_t>(vec);
  EXPECT_EQ(err, pando::Status::Success);
  EXPECT_EQ(graph.size(), SIZE);
  std::uint64_t id = 0;
  for (typename galois::graph_traits<Graph>::VertexTopologyID vert : graph.vertices()) {
    EXPECT_EQ(id, graph.getVertexIndex(vert));
    id++;
  }
  graph.deinitialize();
}
