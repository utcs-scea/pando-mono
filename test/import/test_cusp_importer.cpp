// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <numeric>

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

  pando::Array<galois::WMDEdge> edges;
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
  galois::PerHost<pando::Vector<pando::Vector<galois::WMDEdge>>> perHostLocalEdges{};
  EXPECT_EQ(perHostLocalEdges.initialize(), pando::Status::Success);
  uint64_t i = 0;
  for (pando::GlobalRef<pando::Vector<pando::Vector<galois::WMDEdge>>> val : perHostLocalEdges) {
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

  galois::internal::buildEdgeCountToSend(numVirtualHosts, perHostLocalEdges, *edgeCounts);

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

    pando::GlobalPtr<galois::PerHost<pando::Vector<pando::Vector<galois::WMDEdge>>>>
        perHostLocalEdges;
    pando::LocalStorageGuard(perHostLocalEdges, 1);
    err = localEdges.hostFlatten(*perHostLocalEdges);
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> edgeCounts;
    pando::LocalStorageGuard edgeCountsGuard(edgeCounts, 1);

    galois::internal::buildEdgeCountToSend<galois::WMDEdge>(numVirtualHosts, *perHostLocalEdges,
                                                            *edgeCounts);

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

TEST(BuildVirtualToPhysicalMappings, SmallTest) {
  constexpr std::uint64_t numVirtualHosts = 1024;
  constexpr std::uint64_t numHostsMax = 32;

  using UPair = galois::Pair<std::uint64_t, std::uint64_t>;

  pando::Status err;
  pando::Array<UPair> virtHosts;
  err = virtHosts.initialize(numVirtualHosts);
  EXPECT_EQ(err, pando::Status::Success);

  virtHosts[0] = UPair{numVirtualHosts - 1, 0};
  for (std::uint64_t i = 1; i < numVirtualHosts; i++) {
    virtHosts[i] = UPair{1, i};
  }

  pando::GlobalPtr<pando::Array<std::uint64_t>> virtualToPhysicalMapping;
  pando::LocalStorageGuard vTPMGuard(virtualToPhysicalMapping, 1);

  for (std::uint64_t numHosts = 1; numHosts < numHostsMax; numHosts++) {
    err = galois::internal::buildVirtualToPhysicalMapping(numHosts, virtHosts,
                                                          virtualToPhysicalMapping);
    EXPECT_EQ(err, pando::Status::Success);
    if (numHosts == 1) {
      for (std::uint64_t i = numVirtualHosts; i < numVirtualHosts; i++) {
        EXPECT_EQ(static_cast<std::uint64_t>(0), fmap(*virtualToPhysicalMapping, operator[], 0));
      }
    } else {
      EXPECT_EQ(static_cast<std::uint64_t>(0), fmap(*virtualToPhysicalMapping, operator[], 0));
      std::uint64_t j = 0;
      for (std::uint64_t i = numVirtualHosts - 1; i > 0; i--, j++) {
        EXPECT_EQ(1 + (j % (numHosts - 1)), fmap(*virtualToPhysicalMapping, operator[], i));
      }
    }
    liftVoid(*virtualToPhysicalMapping, deinitialize);
  }
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

TEST(BuildVertexPartition, SmallTest) {
  constexpr std::uint64_t numVirtualHosts = 64;
  constexpr std::uint64_t numVertices = 1'000;
  pando::Status err;

  galois::PerHost<pando::Vector<galois::WMDVertex>> readPartitions{};
  err = readPartitions.initialize();
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < readPartitions.size(); i++) {
    err = fmap(readPartitions.get(i), initialize, numVertices);
    EXPECT_EQ(err, pando::Status::Success);
  }

  const std::uint64_t numHosts = readPartitions.size();

  err = galois::doAll(
      readPartitions, +[](pando::GlobalRef<pando::Vector<galois::WMDVertex>> vec) {
        auto err = galois::doAll(
            &vec, galois::IotaRange(0, numVertices),
            +[](pando::GlobalPtr<pando::Vector<galois::WMDVertex>> vecPtr, std::uint64_t i) {
              fmap(*vecPtr, get, i) = genVertex(i);
            });
        EXPECT_EQ(err, pando::Status::Success);
      });
  EXPECT_EQ(err, pando::Status::Success);

  pando::Array<std::uint64_t> virtualToPhysicalMapping;
  err = virtualToPhysicalMapping.initialize(numVirtualHosts);
  EXPECT_EQ(err, pando::Status::Success);

  std::uint64_t hostID = 0;
  for (pando::GlobalRef<std::uint64_t> val : virtualToPhysicalMapping) {
    val = hostID % numHosts;
    hostID++;
  }

  galois::PerHost<galois::PerHost<pando::Vector<galois::WMDVertex>>> partVert{};
  err = partVert.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  struct PHPV {
    pando::Array<std::uint64_t> v2PM;
    galois::PerHost<pando::Vector<galois::WMDVertex>> pHV;
  };

  auto f =
      +[](PHPV phpv, pando::GlobalRef<galois::PerHost<pando::Vector<galois::WMDVertex>>> partVert) {
        const std::uint64_t hostID = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
        auto err = galois::internal::perHostPartitionVertex<galois::WMDVertex>(
            phpv.v2PM, phpv.pHV.get(hostID), &partVert);
        EXPECT_EQ(err, pando::Status::Success);
      };

  err = galois::doAll(PHPV{virtualToPhysicalMapping, readPartitions}, partVert, f);
  EXPECT_EQ(err, pando::Status::Success);

  err = galois::doAll(
      partVert, +[](pando::GlobalRef<galois::PerHost<pando::Vector<galois::WMDVertex>>> pHVRef) {
        galois::PerHost<pando::Vector<galois::WMDVertex>> pHV = pHVRef;
        auto err = galois::doAll(
            pHV, +[](pando::Vector<galois::WMDVertex> vertices) {
              const std::uint64_t numHosts =
                  static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
              const std::uint64_t hostID =
                  static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
              for (galois::WMDVertex vert : vertices) {
                EXPECT_EQ(((vert.id) % numVirtualHosts) % numHosts, hostID);
              }
              vertices.deinitialize();
            });
        EXPECT_EQ(err, pando::Status::Success);
      });
  EXPECT_EQ(err, pando::Status::Success);

  for (galois::PerHost<pando::Vector<galois::WMDVertex>> pVV : partVert) {
    pVV.deinitialize();
  }
  partVert.deinitialize();

  for (pando::Vector<galois::WMDVertex> vec : readPartitions) {
    vec.deinitialize();
  }
  readPartitions.deinitialize();
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

    pando::GlobalPtr<galois::PerHost<pando::Vector<pando::Vector<galois::WMDEdge>>>>
        perHostLocalEdges;
    pando::LocalStorageGuard(perHostLocalEdges, 1);
    err = localEdges.hostFlatten(*perHostLocalEdges);
    EXPECT_EQ(err, pando::Status::Success);

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> edgeCounts;
    pando::LocalStorageGuard edgeCountsGuard(edgeCounts, 1);

    galois::internal::buildEdgeCountToSend<galois::WMDEdge>(numVirtualHosts, *perHostLocalEdges,
                                                            *edgeCounts);

    for (std::uint64_t numHosts = 2; numHosts <= numVirtualHosts; numHosts *= 9) {
      err = galois::internal::buildVirtualToPhysicalMapping(numHosts, *edgeCounts,
                                                            virtualToPhysicalMapping);
      if (numHosts == 1) {
        for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
          EXPECT_EQ(static_cast<std::uint64_t>(0), fmap(*virtualToPhysicalMapping, operator[], i));
        }
      } else {
        pando::Array<std::uint64_t> interArray;
        err = interArray.initialize(numHosts);
        interArray.fill(0);

        // Rebuild intermediate array
        for (galois::WMDEdge edge : edges) {
          std::uint64_t i = fmap(*virtualToPhysicalMapping, operator[], edge.src % numVirtualHosts);
          interArray[i] += 1;
        }
        // Find that min and max differ by no more than 2*SIZE

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
      liftVoid(*virtualToPhysicalMapping, deinitialize);
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
