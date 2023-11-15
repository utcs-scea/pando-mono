// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
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

TEST(DistArrayCSR, DISABLED_FullyConnected) {
  // This paradigm is necessary because the CP system is not part of the PGAS system
  pando::Notification necessary;
  EXPECT_EQ(necessary.init(), pando::Status::Success);

  auto f = +[](pando::Notification::HandleType done) {
    constexpr std::uint64_t SIZE = 100;
    constexpr std::uint64_t value = 0xDEADBEEF;
    galois::DistArrayCSR<std::uint64_t, std::uint64_t> graph;
    auto vec = generateFullyConnectedGraph(SIZE);
    graph.initialize(vec);
    auto err = deleteVectorVector<std::uint64_t>(vec);
    EXPECT_EQ(err, pando::Status::Success);
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_EQ(graph.getNumEdges(i), SIZE);
      graph.setData(i, value);
      for (std::uint64_t j = 0; j < SIZE; j++) {
        EXPECT_EQ(graph.getEdgeDst(i, j), j);
        graph.setEdgeData(i, j, value);
      }
    }
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_EQ(graph.getNumEdges(i), SIZE);
      EXPECT_EQ(graph.getData(i), value);
      for (std::uint64_t j = 0; j < SIZE; j++) {
        EXPECT_EQ(graph.getEdgeDst(i, j), j);
        EXPECT_EQ(graph.getEdgeData(i, j), value);
      }
    }
    graph.deinitialize();

    done.notify();
  };

  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             necessary.getHandle()),
            pando::Status::Success);
  necessary.wait();
}

TEST(DistArrayCSR, DISABLED_TopologyIteratorsFor) {
  constexpr std::uint64_t SIZE = 100;
  auto vec = generateFullyConnectedGraph(SIZE);
  galois::DistArrayCSR<std::uint64_t, std::uint64_t> graph;
  graph.initialize(vec);
  auto err = deleteVectorVector<std::uint64_t>(std::move(vec));
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

using Graph = galois::DistArrayCSR<std::uint64_t, std::uint64_t>;

struct GraphBools {
  Graph graph;
  pando::GlobalPtr<bool> ptr;
};

TEST(DistArrayCSR, DISABLED_TopologyVertexIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 100;
  auto vec = generateFullyConnectedGraph(SIZE);
  GraphBools gBools;
  gBools.graph.initialize(vec);
  auto err = deleteVectorVector<std::uint64_t>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<bool> touchedBools;
  pando::LocalStorageGuard touchedBoolsGuard(touchedBools, SIZE);
  for (std::uint64_t i = 0; i < SIZE; i++) {
    touchedBools[i] = false;
  }
  gBools.ptr = touchedBools;

  galois::doAll(
      gBools, gBools.graph.vertices(), +[](GraphBools g, typename Graph::VertexTopologyID vlid) {
        constexpr std::uint64_t SIZE = 100;
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

TEST(DistArrayCSR, DISABLED_TopologyEdgeIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 100;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<std::uint64_t>(std::move(vec));
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

TEST(DistArrayCSR, DISABLED_DataVertexIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 100;
  constexpr std::uint64_t goodValue = 0xDEADBEEF;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<std::uint64_t>(std::move(vec));
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

TEST(DistArrayCSR, DISABLED_DataEdgeIteratorsDoAll) {
  constexpr std::uint64_t SIZE = 100;
  constexpr std::uint64_t goodValue = 0xDEADBEEF;
  auto vec = generateFullyConnectedGraph(SIZE);
  Graph g;
  g.initialize(vec);
  auto err = deleteVectorVector<std::uint64_t>(std::move(vec));
  EXPECT_EQ(err, pando::Status::Success);

  for (typename Graph::VertexTopologyID vlid : g.vertices()) {
    galois::doAll(
        goodValue, g.edgeDataRange(vlid),
        +[](std::uint64_t goodValue, pando::GlobalRef<std::uint64_t> eData) {
          eData = goodValue;
        });
  }

  for (std::uint64_t i = 0; i < g.size(); i++) {
    for (std::uint64_t j = 0; j < g.getNumEdges(i); j++) {
      EXPECT_EQ(g.getEdgeData(i, j), goodValue);
    }
  }

  g.deinitialize();
}
