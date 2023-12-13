// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-rt/memory/memory_guard.hpp>

TEST(InsertLocalEdgesPerThread, SingleInsertionTest) {
  pando::Status err;
  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;

  pando::LocalStorageGuard hashGuard(hashPtr, 1);
  *hashPtr = galois::HashTable<std::uint64_t, std::uint64_t>();

  err = fmap(*hashPtr, initialize, 0);
  EXPECT_EQ(err, pando::Status::Success);

  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::WMDEdge e0(0, 1, agile::TYPES::HASORG, agile::TYPES::PUBLICATION, agile::TYPES::TOPIC);

  auto f = +[](pando::NotificationHandle done,
               pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr,
               galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges,
               galois::WMDEdge edge) {
    pando::Status err;
    err = galois::internal::insertLocalEdgesPerThread(*hashPtr, localEdges, edge);
    EXPECT_EQ(err, pando::Status::Success);
    done.notify();
  };

  pando::Notification done;
  EXPECT_EQ(done.init(), pando::Status::Success);

  pando::Place place = pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};
  err = pando::executeOn(place, f, done.getHandle(), hashPtr, localEdges, e0);

  done.wait();

  EXPECT_EQ(err, pando::Status::Success);

  std::uint64_t src = 0xDEADBEEF;
  EXPECT_TRUE(fmap(*hashPtr, get, e0.src, src));
  EXPECT_EQ(e0.src, src);
  EXPECT_EQ(lift(*hashPtr, size), 1);
  EXPECT_EQ(localEdges.sizeAll(), 1);
}

TEST(InsertLocalEdgesPerThread, MultiSmallInsertionTest) {
  constexpr std::uint64_t SIZE = 100;
  pando::Status err;

  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    *hashPtr = galois::HashTable<std::uint64_t, std::uint64_t>();
    err = fmap(*hashPtr, initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  pando::Array<galois::WMDEdge> edges;
  err = edges.initialize(SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    galois::WMDEdge e0(i, i + 1, agile::TYPES::HASORG, agile::TYPES::PUBLICATION,
                       agile::TYPES::TOPIC);
    edges[i] = e0;
  }

  struct State {
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  };
  auto f = +[](State s, galois::WMDEdge edge) {
    pando::Status err;
    err = galois::internal::insertLocalEdgesPerThread(s.hashPtr[s.localEdges.getLocalVectorID()],
                                                      s.localEdges, edge);
    EXPECT_EQ(err, pando::Status::Success);
  };

  err = galois::doAll(State{hashPtr, localEdges}, edges, f);
  EXPECT_EQ(err, pando::Status::Success);

  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    sum += lift(hashPtr[i], size);
  }

  EXPECT_EQ(sum, SIZE);
  EXPECT_EQ(localEdges.sizeAll(), SIZE);

  pando::Array<bool> correctSrc;
  pando::Array<bool> correctDst;
  err = correctSrc.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  err = correctDst.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  std::fill(correctSrc.begin(), correctSrc.end(), false);
  std::fill(correctDst.begin(), correctDst.end(), false);

  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
    pando::Vector<pando::Vector<galois::WMDEdge>> vec = *localEdges.get(i);
    for (typename decltype(table)::Entry e : table) {
      EXPECT_FALSE(correctSrc[e.key]);
      correctSrc[e.key] = true;
      ASSERT_EQ(lift(vec[e.value], size), 1);
      galois::WMDEdge edge = fmap(vec[e.value], get, 0);
      EXPECT_FALSE(correctDst[edge.dst - 1]);
      correctDst[edge.dst - 1] = true;
    }
  }

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_TRUE(correctSrc[i]);
    EXPECT_TRUE(correctDst[i]);
  }

  localEdges.deinitialize();
}

TEST(InsertLocalEdgesPerThread, MultiBigInsertionTest) {
  constexpr std::uint64_t SIZE = 100;
  pando::Status err;

  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    *hashPtr = galois::HashTable<std::uint64_t, std::uint64_t>();
    err = fmap(*hashPtr, initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  pando::Array<galois::WMDEdge> edges;
  err = edges.initialize(SIZE * SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    for (std::uint64_t j = 0; j < SIZE; j++) {
      galois::WMDEdge e0(i, j, agile::TYPES::HASORG, agile::TYPES::PUBLICATION,
                         agile::TYPES::TOPIC);
      edges[i] = e0;
    }
  }

  struct State {
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  };
  auto f = +[](State s, galois::WMDEdge edge) {
    pando::Status err;
    err = galois::internal::insertLocalEdgesPerThread(s.hashPtr[s.localEdges.getLocalVectorID()],
                                                      s.localEdges, edge);
    EXPECT_EQ(err, pando::Status::Success);
  };

  err = galois::doAll(State{hashPtr, localEdges}, edges, f);
  EXPECT_EQ(err, pando::Status::Success);

  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    sum += lift(hashPtr[i], size);
  }

  EXPECT_TRUE(sum >= SIZE);
  EXPECT_EQ(localEdges.sizeAll(), SIZE);

  pando::Array<std::uint64_t> correctSrc;
  pando::Array<std::uint64_t> correctDst;
  err = correctSrc.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  err = correctDst.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  std::fill(correctSrc.begin(), correctSrc.end(), 0);
  std::fill(correctDst.begin(), correctDst.end(), 0);

  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
    pando::Vector<pando::Vector<galois::WMDEdge>> vec = *localEdges.get(i);
    for (typename decltype(table)::Entry e : table) {
      EXPECT_LT(correctSrc[e.key], SIZE);
      correctSrc[e.key] += 1;
      EXPECT_LT(lift(vec[e.value], size), SIZE + 1);
      galois::WMDEdge edge = fmap(vec[e.value], get, 0);
      EXPECT_LT(correctDst[edge.dst], SIZE);
      correctDst[edge.dst] += 1;
    }
  }

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(correctSrc[i], SIZE);
    EXPECT_EQ(correctDst[i], SIZE);
  }

  localEdges.deinitialize();
}
