// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>
#include <tc_naive.hpp>

TEST(NaiveTC, OneTriangle) {
  pando::Notification necessary;
  EXPECT_EQ(necessary.init(), pando::Status::Success);
  auto f = +[](pando::Notification::HandleType done) {
    constexpr std::uint64_t SIZE = 3;
    galois::DistArray<std::uint64_t> array;
    pando::Vector<pando::Vector<galois::ELEdge>> vec;
    EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
    for (pando::GlobalRef<pando::Vector<galois::ELEdge>> edges : vec) {
      pando::Vector<galois::ELEdge> inner;
      EXPECT_EQ(inner.initialize(0), pando::Status::Success);
      edges = inner;
    }

    EXPECT_EQ(galois::doAll(
                  vec,
                  +[](pando::GlobalRef<pando::Vector<galois::ELEdge>> innerRef) {
                    pando::Vector<galois::ELEdge> inner = innerRef;
                    for (std::uint64_t i = 0; i < SIZE; i++) {
                      EXPECT_EQ(inner.pushBack(galois::ELEdge{0, i}), pando::Status::Success);
                    }
                    innerRef = inner;
                  }),
              pando::Status::Success);
    galois::DistArrayCSR<galois::ELVertex, galois::ELEdge> graph;
    graph.initialize(vec);
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_EQ(graph.getNumEdges(i), SIZE);
      for (std::uint64_t j = 0; j < SIZE; j++) {
        EXPECT_EQ(graph.getEdgeDst(i, j), j);
      }
    }

    pando::GlobalPtr<galois::DistArrayCSR<galois::ELVertex, galois::ELEdge>> graphPtr = &graph;

    std::uint64_t count = 0;
    pando::GlobalPtr<std::uint64_t> countPtr = &count;
    EXPECT_EQ(galois::DirOptNaiveTC(graphPtr, countPtr), pando::Status::Success);
    EXPECT_EQ(count, 1);
    graph.deinitialize();
    done.notify();
  };

  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             necessary.getHandle()),
            pando::Status::Success);
  necessary.wait();
}
