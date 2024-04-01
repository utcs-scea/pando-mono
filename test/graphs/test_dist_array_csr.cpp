// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

using Graph = galois::DistArrayCSR<uint64_t, galois::ELEdge>;

pando::Vector<pando::Vector<galois::ELEdge>> generateFullyConnectedGraph(std::uint64_t SIZE) {
  pando::Vector<pando::Vector<galois::ELEdge>> vec;
  EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
  for (pando::GlobalRef<pando::Vector<galois::ELEdge>> edges : vec) {
    pando::Vector<galois::ELEdge> inner;
    EXPECT_EQ(inner.initialize(0), pando::Status::Success);
    edges = inner;
  }

  galois::doAll(
      SIZE, vec, +[](std::uint64_t size, pando::GlobalRef<pando::Vector<galois::ELEdge>> innerRef) {
        pando::Vector<galois::ELEdge> inner = innerRef;
        for (std::uint64_t i = 0; i < size; i++) {
          EXPECT_EQ(inner.pushBack(galois::ELEdge{0, i}), pando::Status::Success);
        }
        innerRef = inner;
      });
  return vec;
}

template <typename T>
pando::Status deleteVectorVector(pando::Vector<pando::Vector<T>> vec) {
  auto err = galois::doAll(
      vec, +[](pando::GlobalRef<pando::Vector<T>> innerRef) {
        pando::Vector<T> inner = innerRef;
        inner.deinitialize();
        innerRef = inner;
      });
  vec.deinitialize();
  return err;
}

TEST(DistArrayCSR, FullyConnected) {
  constexpr std::uint64_t SIZE = 10;
  constexpr std::uint64_t value = 0xDEADBEEF;
  Graph graph;
  auto vec = generateFullyConnectedGraph(SIZE);
  graph.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(vec);
  EXPECT_EQ(err, pando::Status::Success);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(graph.getNumEdges(i), SIZE);
    graph.setData(i, value);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      EXPECT_EQ(graph.getEdgeDst(i, j), j);
      graph.setEdgeData(i, j, galois::ELEdge{i, value});
    }
  }
  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(graph.getNumEdges(i), SIZE);
    EXPECT_EQ(graph.getData(i), value);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      EXPECT_EQ(graph.getEdgeDst(i, j), j);
      galois::ELEdge actual = graph.getEdgeData(i, j);
      EXPECT_EQ(actual.dst, value);
    }
  }
  graph.deinitialize();
}

TEST(DistArrayCSR, TopologyIteratorsFor) {
  constexpr std::uint64_t SIZE = 10;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph graph;
  graph.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  galois::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);

  std::uint64_t i = 0;
  for (std::uint64_t vlid : graph.vertices()) {
    EXPECT_EQ(i, vlid);
    std::uint64_t j = 0;
    for (pando::GlobalRef<std::uint64_t> dstVertex : graph.edges(vlid)) {
      EXPECT_EQ(dstVertex, j);
      j++;
    }
    EXPECT_EQ(j, SIZE);
    i++;
  }
  EXPECT_EQ(i, SIZE);

  i = 0;
  for (galois::DistArraySlice<std::uint64_t> edgeRange : graph.vertices()) {
    std::uint64_t j = 0;
    for (pando::GlobalRef<std::uint64_t> dstVertex : edgeRange) {
      EXPECT_EQ(dstVertex, j);
      j++;
    }
    EXPECT_EQ(j, SIZE);
    i++;
  }
  EXPECT_EQ(i, SIZE);

  graph.deinitialize();
}

struct GraphBools {
  Graph graph;
  pando::GlobalPtr<bool> ptr;
};

TEST(DistArrayCSR, TopologyVertexIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 10;
  auto vec = generateFullyConnectedGraph(SIZE);
  GraphBools gBools;
  gBools.graph.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<bool> touchedBools;
  pando::LocalStorageGuard touchedBoolsGuard(touchedBools, SIZE);
  for (std::uint64_t i = 0; i < SIZE; i++) {
    touchedBools[i] = false;
  }
  gBools.ptr = touchedBools;

  galois::doAll(
      gBools, gBools.graph.vertices(), +[](GraphBools g, typename Graph::VertexTopologyID vlid) {
        constexpr std::uint64_t SIZE = 10;
        g.ptr[vlid] = true;
        std::uint64_t j = 0;
        for (pando::GlobalRef<std::uint64_t> dstVertex : g.graph.edges(vlid)) {
          EXPECT_EQ(dstVertex, j);
          j++;
        }
        EXPECT_EQ(j, SIZE);
      });

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_TRUE(touchedBools[i]);
  }

  gBools.graph.deinitialize();
}

TEST(DistArrayCSR, TopologyEdgeIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 10;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<bool> touchedBools;
  pando::LocalStorageGuard touchedBoolsGuard(touchedBools, SIZE);

  for (typename Graph::EdgeRange eRange : g.vertices()) {
    for (std::uint64_t i = 0; i < SIZE; i++) {
      touchedBools[i] = false;
    }
    galois::doAll(
        touchedBools, eRange,
        +[](pando::GlobalPtr<bool> ptr, pando::GlobalRef<std::uint64_t> dstVertex) {
          ptr[dstVertex] = true;
        });
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_TRUE(touchedBools[i]);
      touchedBools[i] = false;
    }
    galois::doAll(
        GraphBools{g, touchedBools}, eRange, +[](GraphBools gb, std::uint64_t edgeHandle) {
          gb.ptr[gb.graph.getEdgeDst(edgeHandle)] = true;
        });
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_TRUE(touchedBools[i]);
    }
  }
  g.deinitialize();
}

TEST(DistArrayCSR, DataVertexIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 10;
  constexpr std::uint64_t goodValue = 0xDEADBEEF;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  galois::doAll(
      goodValue, g.vertexDataRange(),
      +[](std::uint64_t goodValue, pando::GlobalRef<std::uint64_t> vData) {
        vData = goodValue;
      });

  for (std::uint64_t i = 0; i < g.size(); i++) {
    EXPECT_EQ(g.getData(i), goodValue);
  }

  g.deinitialize();
}

TEST(DistArrayCSR, DataEdgeIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 10;
  constexpr std::uint64_t goodValue = 0xDEADBEEF;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<galois::ELEdge>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  for (typename Graph::VertexTopologyID vlid : g.vertices()) {
    galois::doAll(
        goodValue, g.edgeDataRange(vlid),
        +[](uint64_t goodValue, pando::GlobalRef<galois::ELEdge> eData) {
          eData = galois::ELEdge{goodValue, goodValue};
        });
  }

  for (std::uint64_t i = 0; i < g.size(); i++) {
    for (std::uint64_t j = 0; j < g.getNumEdges(i); j++) {
      galois::ELEdge edge_data = g.getEdgeData(i, j);
      EXPECT_EQ(edge_data.dst, goodValue);
    }
  }

  g.deinitialize();
}
