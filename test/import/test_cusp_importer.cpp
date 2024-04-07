// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <numeric>

#include <pando-lib-galois/containers/thread_local_storage.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-rt/memory/memory_guard.hpp>

uint64_t getNumEdges(const std::string& filename) {
  uint64_t numEdges = 0;
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("//") != std::string::npos) {
      continue;
    } else if (line.find("/*") != std::string::npos || line.find("*/") != std::string::npos) {
      continue;
    } else {
      numEdges++;
    }
  }
  return numEdges;
}

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
    err = galois::internal::insertLocalEdgesPerThread(*hashPtr, localEdges.getThreadVector(), edge);
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

  localEdges.deinitialize();
}

TEST(InsertLocalEdgesPerThread, MultiSmallInsertionTest) {
  constexpr std::uint64_t SIZE = 1000;
  pando::Status err;

  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    hashPtr[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
    err = fmap(hashPtr[i], initialize, 0);
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
                                                      s.localEdges.getThreadVector(), edge);
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

  std::uint64_t edgeCount = 0;
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
    pando::Vector<pando::Vector<galois::WMDEdge>> vec = *localEdges.get(i);
    EXPECT_EQ(table.size(), vec.size());
    for (typename decltype(table)::Entry e : table) {
      EXPECT_FALSE(correctSrc[e.key]);
      correctSrc[e.key] = true;
      ASSERT_EQ(lift(vec[e.value], size), 1);
      galois::WMDEdge edge = fmap(vec[e.value], get, 0);
      EXPECT_EQ(e.key, edge.src);
      EXPECT_EQ(e.key + 1, edge.dst);
      EXPECT_FALSE(correctDst[edge.dst - 1]);
      correctDst[edge.dst - 1] = true;
      edgeCount++;
      liftVoid(vec[e.value], deinitialize);
    }
    table.deinitialize();
  }

  EXPECT_EQ(edgeCount, SIZE);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_TRUE(correctSrc[i]);
    EXPECT_TRUE(correctDst[i]);
  }
  correctSrc.deinitialize();
  correctDst.deinitialize();

  localEdges.deinitialize();
  edges.deinitialize();
}

galois::WMDEdge genEdge(std::uint64_t i, std::uint64_t j, std::uint64_t SIZE) {
  switch ((i * SIZE + j) % 8) {
    case 0:
      return galois::WMDEdge(i, j, agile::TYPES::SALE, agile::TYPES::PERSON, agile::TYPES::PERSON);
    case 1:
      return galois::WMDEdge(i, j, agile::TYPES::AUTHOR, agile::TYPES::PERSON, agile::TYPES::FORUM);
    case 2:
      return galois::WMDEdge(i, j, agile::TYPES::AUTHOR, agile::TYPES::PERSON,
                             agile::TYPES::FORUMEVENT);
    case 3:
      return galois::WMDEdge(i, j, agile::TYPES::AUTHOR, agile::TYPES::PERSON,
                             agile::TYPES::PUBLICATION);
    case 4:
      return galois::WMDEdge(i, j, agile::TYPES::INCLUDES, agile::TYPES::FORUM,
                             agile::TYPES::FORUMEVENT);
    case 5:
      return galois::WMDEdge(i, j, agile::TYPES::HASTOPIC, agile::TYPES::FORUM,
                             agile::TYPES::TOPIC);
    case 6:
      return galois::WMDEdge(i, j, agile::TYPES::HASTOPIC, agile::TYPES::FORUMEVENT,
                             agile::TYPES::TOPIC);
    case 7:
      return galois::WMDEdge(i, j, agile::TYPES::HASTOPIC, agile::TYPES::PUBLICATION,
                             agile::TYPES::TOPIC);
    default:
      return galois::WMDEdge(0, 0, agile::TYPES::NONE, agile::TYPES::NONE, agile::TYPES::NONE);
  }
}

TEST(InsertLocalEdgesPerThread, MultiBigInsertionTest) {
  constexpr std::uint64_t SIZE = 32;
  pando::Status err;

  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    hashPtr[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
    err = fmap(hashPtr[i], initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  galois::DistArray<galois::WMDEdge> edges;
  err = edges.initialize(SIZE * SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    for (std::uint64_t j = 0; j < SIZE; j++) {
      galois::WMDEdge e = genEdge(i, j, SIZE);
      EXPECT_NE(e.type, agile::TYPES::NONE);
      edges[i * SIZE + j] = e;
    }
  }
  pando::Array<std::uint64_t> correctSrc;
  pando::Array<std::uint64_t> correctDst;
  err = correctSrc.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  err = correctDst.initialize(SIZE);
  EXPECT_EQ(err, pando::Status::Success);
  std::fill(correctSrc.begin(), correctSrc.end(), 0);
  std::fill(correctDst.begin(), correctDst.end(), 0);

  for (std::uint64_t i = 0; i < SIZE * SIZE; i++) {
    galois::WMDEdge e = edges[i];
    correctSrc[e.src] += 1;
    correctDst[e.dst] += 1;
  }

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(correctSrc[i], SIZE);
    EXPECT_EQ(correctDst[i], SIZE);
  }

  struct State {
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  };
  auto f = +[](State s, galois::WMDEdge edge) {
    pando::Status err;
    err = galois::internal::insertLocalEdgesPerThread(s.hashPtr[s.localEdges.getLocalVectorID()],
                                                      s.localEdges.getThreadVector(), edge);
    EXPECT_EQ(err, pando::Status::Success);
  };

  err = galois::doAll(State{hashPtr, localEdges}, edges, f);
  EXPECT_EQ(err, pando::Status::Success);

  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    sum += lift(hashPtr[i], size);
  }

  EXPECT_TRUE(sum >= SIZE);
  EXPECT_GT(localEdges.sizeAll() + 1, SIZE);

  std::fill(correctSrc.begin(), correctSrc.end(), 0);
  std::fill(correctDst.begin(), correctDst.end(), 0);

  auto edgeCount = 0;
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
    pando::Vector<pando::Vector<galois::WMDEdge>> vec = *localEdges.get(i);
    for (typename decltype(table)::Entry e : table) {
      EXPECT_LT(correctSrc[e.key], SIZE);
      EXPECT_LT(lift(vec[e.value], size), SIZE + 1);
      EXPECT_GT(lift(vec[e.value], size), 0);
      for (std::uint64_t j = 0; j < lift(vec[e.value], size); j++) {
        galois::WMDEdge edge = fmap(vec[e.value], get, j);
        EXPECT_EQ(edge.src, e.key);
        EXPECT_EQ(edge, genEdge(e.key, edge.dst, SIZE));
        EXPECT_LT(correctDst[edge.dst], SIZE);
        correctSrc[e.key] += 1;
        correctDst[edge.dst] += 1;
        edgeCount++;
      }
      liftVoid(vec[e.value], deinitialize);
    }
    table.deinitialize();
  }

  EXPECT_EQ(edgeCount, SIZE * SIZE);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(correctSrc[i], SIZE);
    EXPECT_EQ(correctDst[i], SIZE);
  }
  correctSrc.deinitialize();
  correctDst.deinitialize();

  localEdges.deinitialize();
}

TEST(BuildEdgeCountToSend, SmallSequentialTest) {
  uint16_t numHosts = pando::getPlaceDims().node.id;
  uint32_t numVirtualHosts = numHosts;

  pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> edgeCounts;
  pando::LocalStorageGuard edgeCountsGuard(edgeCounts, 1);

  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledEdgeCounts;
  galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> perThreadLocalEdges{};
  EXPECT_EQ(perThreadLocalEdges.initialize(), pando::Status::Success);
  auto dims = pando::getPlaceDims();
  auto cores = static_cast<std::uint64_t>(dims.core.x * dims.core.y);
  auto threads = static_cast<std::uint64_t>(pando::getThreadDims().id);
  uint64_t i = 0;
  for (std::uint64_t j = 0; j < perThreadLocalEdges.size(); j += cores * threads) {
    pando::GlobalRef<pando::Vector<pando::Vector<galois::WMDEdge>>> val =
        *perThreadLocalEdges.get(j);
    galois::WMDVertex v0(i, agile::TYPES::PERSON);
    galois::WMDVertex v1(numHosts + i, agile::TYPES::PUBLICATION);
    galois::WMDEdge edge0(v0.id, v1.id, agile::TYPES::AUTHOR, v0.type, v1.type);
    pando::Vector<pando::Vector<galois::WMDEdge>> localEdges;
    pando::Vector<galois::WMDEdge> e0;
    PANDO_CHECK(e0.initialize(0));
    PANDO_CHECK(e0.pushBack(edge0));
    PANDO_CHECK(localEdges.initialize(0));
    PANDO_CHECK(localEdges.pushBack(e0));
    val = localEdges;
    i++;
  }

  PANDO_CHECK(
      galois::internal::buildEdgeCountToSend(numVirtualHosts, perThreadLocalEdges, *edgeCounts));

  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> counts = *edgeCounts;
  uint64_t cnt = 1;
  galois::Pair<std::uint64_t, std::uint64_t> p = counts[0];
  EXPECT_EQ(p.first, cnt);
}

TEST(BuildEdgeCountToSend, MultiBigInsertionTest) {
  constexpr std::uint64_t SIZE = 32;
  pando::Status err;
  galois::DistArray<galois::WMDEdge> edges;
  err = edges.initialize(SIZE * SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    for (std::uint64_t j = 0; j < SIZE; j++) {
      galois::WMDEdge e = genEdge(i, j, SIZE);
      EXPECT_NE(e.type, agile::TYPES::NONE);
      edges[i * SIZE + j] = e;
    }
  }
  for (std::uint32_t numVirtualHosts = 2; numVirtualHosts < 128; numVirtualHosts *= 13) {
    galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> localEdges;
    err = localEdges.initialize();
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
    for (std::uint64_t i = 0; i < localEdges.size(); i++) {
      hashPtr[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
      err = fmap(hashPtr[i], initialize, 0);
      EXPECT_EQ(err, pando::Status::Success);
    }

    struct State {
      pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
      galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> localEdges;
    };
    auto f = +[](State s, galois::WMDEdge edge) {
      pando::Status err;
      err = galois::internal::insertLocalEdgesPerThread(
          s.hashPtr[galois::ThreadLocalStorage<std::uint64_t>::getCurrentThreadIdx()],
          s.localEdges.getLocalRef(), edge);
      EXPECT_EQ(err, pando::Status::Success);
    };

    err = galois::doAll(State{hashPtr, localEdges}, edges, f);
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> edgeCounts;
    pando::LocalStorageGuard edgeCountsGuard(edgeCounts, 1);

    PANDO_CHECK(galois::internal::buildEdgeCountToSend<galois::WMDEdge>(numVirtualHosts, localEdges,
                                                                        *edgeCounts));

    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> counts = *edgeCounts;
    std::uint64_t i = 0;
    for (galois::Pair<std::uint64_t, std::uint64_t> count : counts) {
      EXPECT_EQ(count.second, i);
      std::uint64_t j = SIZE / numVirtualHosts;
      j += (SIZE % numVirtualHosts > i) ? 1 : 0;
      EXPECT_EQ(count.first, SIZE * j);
      i++;
    }

    for (std::uint64_t i = 0; i < localEdges.size(); i++) {
      galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
      table.deinitialize();
      pando::Vector<pando::Vector<galois::WMDEdge>> vecVec = *localEdges.get(i);
      for (pando::Vector<galois::WMDEdge> vec : vecVec) {
        vec.deinitialize();
      }
      table.deinitialize();
    }

    localEdges.deinitialize();
    counts.deinitialize();
  }
  edges.deinitialize();
}

galois::WMDVertex genVertex(std::uint64_t i) {
  switch (i % 5) {
    case 0:
      return galois::WMDVertex(i, agile::TYPES::PERSON);
    case 1:
      return galois::WMDVertex(i, agile::TYPES::FORUMEVENT);
    case 2:
      return galois::WMDVertex(i, agile::TYPES::FORUM);
    case 3:
      return galois::WMDVertex(i, agile::TYPES::PUBLICATION);
    case 4:
      return galois::WMDVertex(i, agile::TYPES::TOPIC);
    default:
      return galois::WMDVertex(0, agile::TYPES::NONE);
  }
}

TEST(PartitionEdgesSerially, Serially) {
  constexpr std::uint64_t SIZE = 32;
  pando::Status err;

  std::uint64_t numVirtualHosts = 16;
  std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);

  galois::DistArray<galois::WMDEdge> edges;
  err = edges.initialize(SIZE * SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    for (std::uint64_t j = 0; j < SIZE; j++) {
      galois::WMDEdge e = genEdge(i, j, SIZE);
      EXPECT_NE(e.type, agile::TYPES::NONE);
      edges[i * SIZE + j] = e;
    }
  }

  galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  err = localEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
  pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    hashPtr[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
    err = fmap(hashPtr[i], initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  struct State {
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    galois::PerThreadVector<pando::Vector<galois::WMDEdge>> localEdges;
  };

  auto f = +[](State s, galois::WMDEdge edge) {
    pando::Status err;
    err = galois::internal::insertLocalEdgesPerThread(s.hashPtr[s.localEdges.getLocalVectorID()],
                                                      s.localEdges.getThreadVector(), edge);
    EXPECT_EQ(err, pando::Status::Success);
  };

  err = galois::doAll(State{hashPtr, localEdges}, edges, f);
  EXPECT_EQ(err, pando::Status::Success);

  pando::Array<std::uint64_t> v2PM;
  err = v2PM.initialize(numVirtualHosts);
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    v2PM[i] = i % numHosts;
  }

  galois::HostIndexedMap<pando::Vector<pando::Vector<galois::WMDEdge>>> partitionedEdges{};
  err = partitionedEdges.initialize();
  EXPECT_EQ(err, pando::Status::Success);
  for (pando::GlobalRef<pando::Vector<pando::Vector<galois::WMDEdge>>> vvec : partitionedEdges) {
    err = fmap(vvec, initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  galois::HostIndexedMap<galois::HashTable<std::uint64_t, std::uint64_t>> perHostRename{};
  err = perHostRename.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  auto g = +[](pando::NotificationHandle done, decltype(localEdges) le,
               decltype(v2PM) virtual2Physical, decltype(partitionedEdges) pe,
               decltype(perHostRename) phr) {
    auto err =
        galois::internal::partitionEdgesSerially<galois::WMDEdge>(le, virtual2Physical, pe, phr);
    PANDO_CHECK(err);
    done.notify();
  };
  pando::Notification notify;
  err = notify.init();
  EXPECT_EQ(err, pando::Status::Success);

  auto place = pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};
  err = pando::executeOn(place, g, notify.getHandle(), localEdges, v2PM, partitionedEdges,
                         perHostRename);
  EXPECT_EQ(err, pando::Status::Success);
  notify.wait();

  for (pando::Vector<pando::Vector<galois::WMDEdge>> vVec : partitionedEdges) {
    EXPECT_EQ(vVec.size(), SIZE / numHosts);
    std::sort(vVec.begin(), vVec.end(),
              [](const pando::Vector<galois::WMDEdge>& a,
                 const pando::Vector<galois::WMDEdge>& b) -> bool {
                galois::WMDEdge edgeA = a[0];
                galois::WMDEdge edgeB = b[0];
                return edgeA.src < edgeB.src;
              });
    for (pando::Vector<galois::WMDEdge> vec : vVec) {
      EXPECT_EQ(vec.size(), SIZE);
      galois::WMDEdge edge = vec[0];
      std::uint64_t edgeSrc = edge.src;
      std::cout << "Edge src: " << edgeSrc << std::endl;
      for (galois::WMDEdge val : vec) {
        EXPECT_EQ(val.src, edgeSrc);
      }
      vec.deinitialize();
    }
    vVec.deinitialize();
  }
  partitionedEdges.deinitialize();
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    liftVoid(hashPtr[i], deinitialize);
  }
  for (auto hashRef : perHostRename) {
    liftVoid(hashRef, deinitialize);
  }
  localEdges.deinitialize();
}

TEST(Integration, InsertEdgeCountVirtual2Physical) {
  constexpr std::uint64_t SIZE = 32;
  pando::Status err;

  galois::DistArray<galois::WMDEdge> edges;
  err = edges.initialize(SIZE * SIZE);
  PANDO_CHECK(err);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    for (std::uint64_t j = 0; j < SIZE; j++) {
      galois::WMDEdge e = genEdge(i, j, SIZE);
      EXPECT_NE(e.type, agile::TYPES::NONE);
      edges[i * SIZE + j] = e;
    }
  }

  pando::GlobalPtr<pando::Array<std::uint64_t>> virtualToPhysicalMapping;
  pando::LocalStorageGuard vTPMGuard(virtualToPhysicalMapping, 1);

  for (std::uint32_t numVirtualHosts = 2; numVirtualHosts < 128; numVirtualHosts *= 13) {
    galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> localEdges;
    err = localEdges.initialize();
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
    pando::LocalStorageGuard hashGuard(hashPtr, localEdges.size());
    for (std::uint64_t i = 0; i < localEdges.size(); i++) {
      hashPtr[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
      err = fmap(hashPtr[i], initialize, 0);
      EXPECT_EQ(err, pando::Status::Success);
    }

    struct State {
      pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> hashPtr;
      galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> localEdges;
    };
    auto f = +[](State s, galois::WMDEdge edge) {
      pando::Status err;
      err = galois::internal::insertLocalEdgesPerThread(
          s.hashPtr[galois::ThreadLocalStorage<std::uint64_t>::getCurrentThreadIdx()],
          s.localEdges.getLocalRef(), edge);
      EXPECT_EQ(err, pando::Status::Success);
    };

    err = galois::doAll(State{hashPtr, localEdges}, edges, f);
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> edgeCounts;
    pando::LocalStorageGuard edgeCountsGuard(edgeCounts, 1);

    PANDO_CHECK(galois::internal::buildEdgeCountToSend<galois::WMDEdge>(numVirtualHosts, localEdges,
                                                                        *edgeCounts));

    for (std::uint64_t numHosts = 2; numHosts <= numVirtualHosts; numHosts *= 3) {
      auto [virtualToPhysicalMapping, totEdges] = PANDO_EXPECT_CHECK(
          galois::internal::buildVirtualToPhysicalMapping(numHosts, *edgeCounts));
      totEdges.deinitialize();
      if (numHosts == 1) {
        for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
          EXPECT_EQ(static_cast<std::uint64_t>(0), virtualToPhysicalMapping[i]);
        }
      } else {
        pando::Array<std::uint64_t> interArray;
        err = interArray.initialize(numHosts);
        interArray.fill(0);

        // Rebuild intermediate array
        for (galois::WMDEdge edge : edges) {
          std::uint64_t i = virtualToPhysicalMapping[edge.src % numVirtualHosts];
          interArray[i] += 1;
        }

        std::uint64_t dif = ((SIZE / numVirtualHosts) + 1) * SIZE;
        std::uint64_t max = 0;
        std::uint64_t min = UINT64_MAX;
        std::uint64_t sum = 0;
        for (std::uint64_t val : interArray) {
          min = (val < min) ? val : min;
          max = (val > max) ? val : max;
          sum += val;
        }
        EXPECT_LT(max - min, dif + 1);
        EXPECT_EQ(sum, SIZE * SIZE);
        interArray.deinitialize();
      }
      virtualToPhysicalMapping.deinitialize();
    }
    for (std::uint64_t i = 0; i < localEdges.size(); i++) {
      galois::HashTable<std::uint64_t, std::uint64_t> table = hashPtr[i];
      table.deinitialize();
      pando::Vector<pando::Vector<galois::WMDEdge>> vecVec = *localEdges.get(i);
      for (pando::Vector<galois::WMDEdge> vec : vecVec) {
        vec.deinitialize();
      }
      table.deinitialize();
    }
    localEdges.deinitialize();
  }

  edges.deinitialize();
}

void getNumVerticesAndEdges(std::string& filename, uint64_t& numVertices, uint64_t& numEdges) {
  std::cout << "filename: " << filename << "\n";
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("//") != std::string::npos) {
      continue;
    } else if (line.find("/*") != std::string::npos || line.find("*/") != std::string::npos) {
      continue;
    } else {
      std::istringstream iss(line);
      std::string word;
      std::getline(iss, word, ',');
      if (word == "Person" || word == "Publication" || word == "Forum" || word == "ForumEvent" ||
          word == "Topic")
        numVertices++;
      else if (word == "Author" || word == "Sale" || word == "Includes" || word == "HasTopic" ||
               word == "HasOrg")
        numEdges++;
    }
  }
  return;
}

TEST(loadGraphFilePerThread, loadGraph) {
  uint64_t numThreads = 2;
  uint64_t segmentsPerThread = 1;
  galois::ThreadLocalVector<pando::Vector<galois::WMDEdge>> localEdges;
  EXPECT_EQ(localEdges.initialize(), pando::Status::Success);
  galois::ThreadLocalVector<galois::WMDVertex> localVertices;
  EXPECT_EQ(localVertices.initialize(), pando::Status::Success);
  pando::Array<char> filename;
  std::string wmdFile = "/pando/graphs/simple_wmd.csv";
  PANDO_CHECK(filename.initialize(wmdFile.size()));
  for (uint64_t i = 0; i < wmdFile.size(); i++)
    filename[i] = wmdFile[i];

  galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename;
  PANDO_CHECK(perThreadRename.initialize());
  for (std::uint64_t i = 0; i < perThreadRename.size(); i++) {
    perThreadRename[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
    pando::Status err = fmap(perThreadRename[i], initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  galois::DAccumulator<std::uint64_t> totVerts{};
  EXPECT_EQ(totVerts.initialize(), pando::Status::Success);

  galois::WaitGroup wg;
  EXPECT_EQ(pando::Status::Success, wg.initialize(numThreads));

  auto wgh = wg.getHandle();

  for (uint64_t i = 0; i < numThreads; i++) {
    pando::Place place =
        pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i % pando::getPlaceDims().node.id)},
                     pando::anyPod, pando::anyCore};
    pando::Status err =
        pando::executeOn(place, &galois::loadWMDFilePerThread, wgh, filename, segmentsPerThread,
                         numThreads, i, localEdges, perThreadRename, localVertices, totVerts);
    EXPECT_EQ(err, pando::Status::Success);
  }
  EXPECT_EQ(pando::Status::Success, wg.wait());

  wg.deinitialize();

  auto freeStuff = +[](galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>>
                           perThreadRename) {
    for (galois::HashTable<std::uint64_t, std::uint64_t> hash : perThreadRename) {
      hash.deinitialize();
    }
  };
  perThreadRename.deinitialize();
  EXPECT_EQ(pando::Status::Success, pando::executeOn(pando::anyPlace, freeStuff, perThreadRename));

  uint64_t numVertices = 0;
  uint64_t numEdges = 0;
  getNumVerticesAndEdges(wmdFile, numVertices, numEdges);
  uint64_t vert = 0;
  for (uint64_t i = 0; i < localVertices.size(); i++) {
    pando::Vector<galois::WMDVertex> vec = localVertices[i];
    vert += vec.size();
  }
  uint64_t edges = 0;
  for (uint64_t i = 0; i < localEdges.size(); i++) {
    for (uint64_t j = 0; j < lift(*localEdges.get(i), size); j++) {
      pando::Vector<galois::WMDEdge> vec = fmap(localEdges[i], operator[], j);
      edges += vec.size();
      vec.deinitialize();
    }
  }

  EXPECT_EQ(vert, numVertices);
  EXPECT_EQ(edges, 2 * numEdges);
  totVerts.deinitialize();
  localVertices.deinitialize();
  localEdges.deinitialize();
}

TEST(loadGraphFilePerThread, loadEdgeList) {
  uint64_t segmentsPerThread = 1;
  galois::ThreadLocalVector<pando::Vector<galois::ELEdge>> localEdges;
  EXPECT_EQ(localEdges.initialize(), pando::Status::Success);

  pando::Array<char> filename;
  const std::string edgelistFile = "/pando/graphs/simple.el";
  const std::uint64_t numVertices = 10;
  PANDO_CHECK(filename.initialize(edgelistFile.size()));
  for (uint64_t i = 0; i < edgelistFile.size(); i++)
    filename[i] = edgelistFile[i];

  const std::uint64_t numThreads = localEdges.size() - pando::getPlaceDims().node.id;

  galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename{};
  PANDO_CHECK(perThreadRename.initialize());
  for (std::uint64_t i = 0; i < perThreadRename.size(); i++) {
    perThreadRename[i] = galois::HashTable<std::uint64_t, std::uint64_t>();
    pando::Status err = fmap(perThreadRename[i], initialize, 0);
    EXPECT_EQ(err, pando::Status::Success);
  }

  galois::WaitGroup wg;
  EXPECT_EQ(pando::Status::Success, wg.initialize(numThreads));
  auto wgh = wg.getHandle();
  for (uint64_t i = 0; i < numThreads; i++) {
    std::int16_t nodeId =
        static_cast<std::int16_t>(i % static_cast<std::uint64_t>(pando::getPlaceDims().node.id));
    pando::Place place = pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore};
    pando::Status err =
        pando::executeOn(place, &galois::loadELFilePerThread, wgh, filename, segmentsPerThread,
                         numThreads, i, localEdges, perThreadRename, numVertices);
    EXPECT_EQ(err, pando::Status::Success);
  }
  EXPECT_EQ(wg.wait(), pando::Status::Success);

  for (galois::HashTable<std::uint64_t, std::uint64_t> hash : perThreadRename) {
    hash.deinitialize();
  }
  perThreadRename.deinitialize();

  uint64_t numEdges = getNumEdges(edgelistFile);
  uint64_t edges = 0;
  for (uint64_t i = 0; i < localEdges.size(); i++) {
    for (uint64_t j = 0; j < lift(*localEdges.get(i), size); j++) {
      auto lEij = fmap(*localEdges.get(i), get, j);
      edges += lift(lEij, size);
      liftVoid(lEij, deinitialize);
    }
  }
  EXPECT_EQ(edges, numEdges);
  localEdges.deinitialize();
}
