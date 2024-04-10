// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023 University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/export.h"

#include <pando-bfs-galois/sssp.hpp>
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

TEST(SSSP, FullyConnected) {
  constexpr std::uint64_t SIZE = 100;
  galois::DistArray<std::uint64_t> array;
  pando::Vector<pando::Vector<std::uint64_t>> vec;
  EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
  for (pando::GlobalRef<pando::Vector<std::uint64_t>> edges : vec) {
    pando::Vector<std::uint64_t> inner;
    EXPECT_EQ(inner.initialize(0), pando::Status::Success);
    edges = inner;
  }

  galois::doAll(
      vec, +[](pando::GlobalRef<pando::Vector<std::uint64_t>> innerRef) {
        pando::Vector<std::uint64_t> inner = innerRef;
        for (std::uint64_t i = 0; i < SIZE; i++) {
          EXPECT_EQ(inner.pushBack(i), pando::Status::Success);
        }
        innerRef = inner;
      });
  using Graph = galois::DistArrayCSR<std::uint64_t, std::uint64_t>;
  Graph graph;

  graph.initialize(vec);
  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(graph.getNumEdges(i), SIZE);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      EXPECT_EQ(graph.getEdgeDst(i, j), j);
    }
  }

  using TopID = typename galois::graph_traits<Graph>::VertexTopologyID;
  using LoopStructure = galois::PerHost<pando::Vector<TopID>>;
  LoopStructure phbfs{};
  PANDO_CHECK(phbfs.initialize());

  PANDO_CHECK(galois::doAll(
      phbfs, +[](pando::GlobalRef<pando::Vector<TopID>> vecRef) {
        PANDO_CHECK(fmap(vecRef, initialize, 2));
        liftVoid(vecRef, clear);
      }));
  galois::PerThreadVector<TopID> next;

  PANDO_CHECK(next.initialize());

  EXPECT_EQ(galois::SSSP(graph, 0, next, phbfs), pando::Status::Success);
  EXPECT_EQ(graph.getData(0), static_cast<std::uint64_t>(0));
  for (std::uint64_t i = 1; i < SIZE; i++) {
    EXPECT_EQ(graph.getData(i), static_cast<std::uint64_t>(1));
  }
  graph.deinitialize();
}
