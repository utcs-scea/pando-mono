// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
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
