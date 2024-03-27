// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <random>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

namespace {

template <typename T>
pando::GlobalPtr<T> getGlobalObject() {
  const auto expected =
      pando::allocateMemory<T>(1, pando::getCurrentPlace(), pando::MemoryType::Main);
  EXPECT_EQ(expected.hasValue(), true);
  return expected.value();
}

uint64_t getHostThreads() {
  uint64_t x = pando::getPlaceDims().core.x;
  uint64_t y = pando::getPlaceDims().core.y;
  uint64_t threads = pando::getThreadDims().id;
  return x * y * threads;
}

struct State {
  State() = default;
  State(galois::WaitGroup::HandleType f, galois::DAccumulator<uint64_t> s) : first(f), second(s) {}

  galois::WaitGroup::HandleType first;
  galois::DAccumulator<uint64_t> second;
};

} // namespace

TEST(PerThreadVector, Init) {
  pando::GlobalPtr<galois::PerThreadVector<uint64_t>> perThreadVecPtr =
      getGlobalObject<galois::PerThreadVector<uint64_t>>();
  galois::PerThreadVector<uint64_t> perThreadVec = *perThreadVecPtr;
  EXPECT_EQ(16, pando::getThreadDims().id);
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);
  pando::Vector<uint64_t> work;
  EXPECT_EQ(work.initialize(1), pando::Status::Success);
  work[0] = 9801;
  galois::doAll(
      perThreadVec, work, +[](galois::PerThreadVector<uint64_t> perThreadVec, uint64_t x) {
        EXPECT_GE(pando::getCurrentThread().id, 0);
        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);
        pando::Vector<uint64_t> localVec = perThreadVec.getThreadVector();
        EXPECT_EQ(localVec.size(), 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), 1);

  uint64_t elts = 0;
  for (pando::Vector<uint64_t> vec : perThreadVec) {
    elts += vec.size();
  }
  EXPECT_EQ(elts, 1);

  *perThreadVecPtr = perThreadVec;
  galois::DistArray<uint64_t> copy;
  EXPECT_EQ(perThreadVec.assign(copy), pando::Status::Success);
  EXPECT_EQ(copy.size(), 1);
  uint64_t val = copy[0];
  EXPECT_EQ(val, 9801);

  copy.deinitialize();
  work.deinitialize();
  perThreadVec.deinitialize();
}

TEST(PerThreadVector, Parallel) {
  pando::GlobalPtr<galois::PerThreadVector<uint64_t>> perThreadVecPtr =
      getGlobalObject<galois::PerThreadVector<uint64_t>>();
  galois::PerThreadVector<uint64_t> perThreadVec = *perThreadVecPtr;
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);

  static const uint64_t workItems = 1000;
  pando::Vector<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  galois::doAll(
      perThreadVec, work, +[](galois::PerThreadVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getThreadVector();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getThreadVector();
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  uint64_t elts = 0;
  for (uint64_t i = 0; i < perThreadVec.size(); i++) {
    pando::Vector<uint64_t> vec = *perThreadVec.get(i);
    elts += vec.size();
    for (uint64_t i = 0; i < vec.size(); i++) {
      EXPECT_LT(vec[i], workItems);
    }
    if (i > getHostThreads()) {
      EXPECT_EQ(vec.size(), 0);
    }
  }
  EXPECT_EQ(elts, workItems);

  *perThreadVecPtr = perThreadVec;
  galois::DistArray<uint64_t> copy;
  EXPECT_EQ(perThreadVec.assign(copy), pando::Status::Success);
  EXPECT_EQ(copy.size(), workItems);

  copy.deinitialize();
  work.deinitialize();
  perThreadVec.deinitialize();
}

TEST(PerThreadVector, DoAll) {
  pando::GlobalPtr<galois::PerThreadVector<uint64_t>> perThreadVecPtr =
      getGlobalObject<galois::PerThreadVector<uint64_t>>();
  galois::PerThreadVector<uint64_t> perThreadVec;
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);
  *perThreadVecPtr = perThreadVec;

  static const uint64_t workItems = 1000;
  galois::DistArray<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  for (uint64_t i = 0; i < workItems; i++) {
    work[i] = i;
  }

  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      perThreadVec, work, +[](galois::PerThreadVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getThreadVector();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getThreadVector();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(perThreadVec.m_indices[perThreadVec.m_indices.size() - 1], perThreadVec.sizeAll());

  galois::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  galois::doAll(
      wg.getHandle(), State(wg.getHandle(), sum), perThreadVec,
      +[](State state, pando::GlobalRef<pando::Vector<uint64_t>> vec) {
        pando::Vector<uint64_t> v = vec;
        for (uint64_t i = 0; i < v.size(); i++) {
          EXPECT_LT(v[i], workItems);
        }
        galois::doAll(
            state.first, state.second, v, +[](galois::DAccumulator<uint64_t> sum, uint64_t ref) {
              EXPECT_LT(ref, workItems);
              sum.add(ref);
            });
      });
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(sum.reduce(), ((workItems - 1) + 0) * (workItems / 2));

  galois::DistArray<uint64_t> copy;
  EXPECT_EQ(perThreadVec.assign(copy), pando::Status::Success);
  EXPECT_EQ(copy.size(), workItems);
  uint64_t copy_sum = 0;
  for (uint64_t elt : copy) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  copy.deinitialize();
  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.deinitialize();
}

TEST(PerThreadVector, HostIndexedMapVector) {
  constexpr std::uint64_t size = 32;
  pando::Status err;

  galois::PerThreadVector<std::uint64_t> ptv;
  err = ptv.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostIndexedMap<std::uint64_t> phu{};

  galois::doAll(
      ptv, phu, +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t) {
        galois::doAll(
            ptv, galois::IotaRange(0, size),
            +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t i) {
              pando::Status err;
              err = ptv.pushBack(i);
              EXPECT_EQ(err, pando::Status::Success);
            });
      });

  pando::GlobalPtr<galois::HostIndexedMap<pando::Vector<std::uint64_t>>> phv;

  pando::LocalStorageGuard memGuard(phv, 1);

  err = ptv.hostFlatten(*phv);
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostIndexedMap<pando::Vector<std::uint64_t>> phvNoRef = *phv;

  for (pando::GlobalRef<pando::Vector<std::uint64_t>> vecRef : phvNoRef) {
    EXPECT_EQ(lift(vecRef, size), size);
    std::sort(lift(vecRef, begin), lift(vecRef, end));
    pando::Vector<std::uint64_t> vec = vecRef;
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(vec[i], i);
    }
  }
}

TEST(PerThreadVector, HostIndexedMapVectorAppend) {
  constexpr std::uint64_t size = 32;
  pando::Status err;

  galois::PerThreadVector<std::uint64_t> ptv;
  err = ptv.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostIndexedMap<std::uint64_t> phu{};

  galois::doAll(
      ptv, phu, +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t) {
        galois::doAll(
            ptv, galois::IotaRange(0, size),
            +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t i) {
              pando::Status err;
              err = ptv.pushBack(i);
              EXPECT_EQ(err, pando::Status::Success);
            });
      });

  galois::HostIndexedMap<pando::Vector<std::uint64_t>> phv{};
  EXPECT_EQ(phv.initialize(), pando::Status::Success);

  for (std::int16_t i = 0; i < static_cast<std::int16_t>(phv.getNumHosts()); i++) {
    auto place = pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore};
    auto ref = phv.get(i);
    EXPECT_EQ(fmap(ref, initialize, 0, place, pando::MemoryType::Main), pando::Status::Success);
  }

  err = ptv.hostFlattenAppend(phv);
  EXPECT_EQ(err, pando::Status::Success);

  for (pando::GlobalRef<pando::Vector<std::uint64_t>> vecRef : phv) {
    EXPECT_EQ(lift(vecRef, size), size);
    std::sort(lift(vecRef, begin), lift(vecRef, end));
    pando::Vector<std::uint64_t> vec = vecRef;
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(vec[i], i);
    }
  }
  phv.deinitialize();
}

TEST(PerThreadVector, Clear) {
  constexpr std::uint64_t size = 32;
  pando::Status err;

  galois::PerThreadVector<std::uint64_t> ptv;
  err = ptv.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostIndexedMap<std::uint64_t> phu{};

  galois::doAll(
      ptv, phu, +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t) {
        galois::doAll(
            ptv, galois::IotaRange(0, size),
            +[](galois::PerThreadVector<std::uint64_t> ptv, std::uint64_t i) {
              pando::Status err;
              err = ptv.pushBack(i);
              EXPECT_EQ(err, pando::Status::Success);
            });
      });

  galois::DAccumulator<std::uint64_t> accum;
  err = lift(accum, initialize);
  EXPECT_EQ(err, pando::Status::Success);

  err = galois::doAll(
      accum, ptv,
      +[](galois::DAccumulator<std::uint64_t> accum,
          pando::GlobalRef<pando::Vector<std::uint64_t>> refVec) {
        accum.add(lift(refVec, size));
      });
  EXPECT_EQ(err, pando::Status::Success);
  EXPECT_EQ(accum.reduce(), size * static_cast<std::uint64_t>(pando::getPlaceDims().node.id));

  ptv.clear();

  galois::doAll(
      ptv, +[](pando::GlobalRef<pando::Vector<std::uint64_t>> refVec) {
        EXPECT_EQ(0, lift(refVec, size));
      });

  accum.deinitialize();
  ptv.deinitialize();
}

TEST(PerThreadVector, ClearCompute) {
  pando::GlobalPtr<galois::PerThreadVector<uint64_t>> perThreadVecPtr =
      getGlobalObject<galois::PerThreadVector<uint64_t>>();
  galois::PerThreadVector<uint64_t> perThreadVec;
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);
  *perThreadVecPtr = perThreadVec;

  static uint64_t workItems = 1000;
  galois::DistArray<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  for (uint64_t i = 0; i < workItems; i++) {
    work[i] = i;
  }

  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      perThreadVec, work, +[](galois::PerThreadVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getThreadVector();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getThreadVector();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(perThreadVec.m_indices[perThreadVec.m_indices.size() - 1], perThreadVec.sizeAll());

  galois::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  galois::doAll(
      wg.getHandle(), State(wg.getHandle(), sum), perThreadVec,
      +[](State state, pando::GlobalRef<pando::Vector<uint64_t>> vec) {
        pando::Vector<uint64_t> v = vec;
        for (uint64_t i = 0; i < v.size(); i++) {
          EXPECT_LT(v[i], workItems);
        }
        galois::doAll(
            state.first, state.second, v, +[](galois::DAccumulator<uint64_t> sum, uint64_t ref) {
              EXPECT_LT(ref, workItems);
              sum.add(ref);
            });
      });
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(sum.reduce(), ((workItems - 1) + 0) * (workItems / 2));

  galois::DistArray<uint64_t> copy;
  EXPECT_EQ(perThreadVec.assign(copy), pando::Status::Success);
  EXPECT_EQ(copy.size(), workItems);
  uint64_t copy_sum = 0;
  for (uint64_t elt : copy) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  copy.deinitialize();
  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.clear();

  workItems = 100;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  for (uint64_t i = 0; i < workItems; i++) {
    work[i] = i;
  }

  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      perThreadVec, work, +[](galois::PerThreadVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getThreadVector();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getThreadVector();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(perThreadVec.m_indices[perThreadVec.m_indices.size() - 1], perThreadVec.sizeAll());

  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  galois::doAll(
      wg.getHandle(), State(wg.getHandle(), sum), perThreadVec,
      +[](State state, pando::GlobalRef<pando::Vector<uint64_t>> vec) {
        pando::Vector<uint64_t> v = vec;
        for (uint64_t i = 0; i < v.size(); i++) {
          EXPECT_LT(v[i], workItems);
        }
        galois::doAll(
            state.first, state.second, v, +[](galois::DAccumulator<uint64_t> sum, uint64_t ref) {
              EXPECT_LT(ref, workItems);
              sum.add(ref);
            });
      });
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(sum.reduce(), ((workItems - 1) + 0) * (workItems / 2));

  EXPECT_EQ(perThreadVec.assign(copy), pando::Status::Success);
  EXPECT_EQ(copy.size(), workItems);
  copy_sum = 0;
  for (uint64_t elt : copy) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  copy.deinitialize();
  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.deinitialize();
}

TEST(Vector, IntVectorOfVectorsUniform) {
  pando::Vector<pando::Vector<std::uint64_t>> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  uint64_t size = 2000;
  galois::HashTable<uint64_t, uint64_t> table;
  PANDO_CHECK(table.initialize(8));
  uint64_t result = 0;

  // Creates a vector of vectors of size [i,1]
  for (uint64_t i = 0; i < size; i++) {
    EXPECT_FALSE(fmap(table, get, i, result));
    PANDO_CHECK(fmap(table, put, i, lift(vec, size)));
    pando::Vector<std::uint64_t> v;
    EXPECT_EQ(v.initialize(1), pando::Status::Success);
    v[0] = i;
    EXPECT_EQ(vec.pushBack(v), pando::Status::Success);
  }

  // Pushes back i+i to each vector
  for (uint64_t i = 0; i < size; i++) {
    EXPECT_TRUE(fmap(table, get, i, result));
    pando::GlobalRef<pando::Vector<uint64_t>> vec1 = fmap(vec, get, result);
    pando::Vector<uint64_t> vec2 = vec1;
    EXPECT_EQ(vec2.get(0), i);
    EXPECT_EQ(fmap(vec1, pushBack, (i + i)), pando::Status::Success);
  }

  // Validates the vectors
  for (uint64_t i = 0; i < size; i++) {
    pando::GlobalRef<pando::Vector<uint64_t>> vec1 = vec.get(i);
    pando::Vector<uint64_t> vec2 = vec1;
    EXPECT_EQ(vec2[1], i + i);
    EXPECT_EQ(vec2[0], i);
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_TRUE(fmap(table, get, i, result));
    EXPECT_EQ(result, i);
  }
  EXPECT_EQ(vec.size(), size);
  vec.deinitialize();
}

TEST(Vector, IntVectorOfVectorsRandom) {
  pando::Vector<pando::Vector<std::uint64_t>> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  uint64_t size = 2000;
  galois::HashTable<uint64_t, uint64_t> table;
  PANDO_CHECK(table.initialize(8));
  uint64_t result = 0;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> distribution(0, size);
  std::unordered_map<uint64_t, std::vector<uint64_t>> map;

  // Creates a vector of vectors by pushing back `dst` to index `table[src]`
  for (uint64_t i = 0; i < size * 4; i++) {
    int src = distribution(gen);
    int dst = distribution(gen);
    if (map.find(src) == map.end()) {
      std::vector<uint64_t> v;
      v.push_back(dst);
      map[src] = v;
    } else {
      map[src].push_back(dst);
    }
    if (fmap(table, get, src, result)) {
      pando::GlobalRef<pando::Vector<uint64_t>> vec1 = fmap(vec, get, result);
      pando::Vector<uint64_t> vec2 = vec1;
      EXPECT_EQ(fmap(vec1, pushBack, dst), pando::Status::Success);
    } else {
      PANDO_CHECK(fmap(table, put, src, lift(vec, size)));
      pando::Vector<std::uint64_t> v;
      EXPECT_EQ(v.initialize(1), pando::Status::Success);
      v[0] = dst;
      EXPECT_EQ(vec.pushBack(v), pando::Status::Success);
    }
  }

  // Validates the vectors
  for (auto it = map.begin(); it != map.end(); ++it) {
    EXPECT_TRUE(fmap(table, get, it->first, result));
    pando::GlobalRef<pando::Vector<uint64_t>> vec1 = fmap(vec, get, result);
    pando::Vector<uint64_t> vec2 = vec1;
    std::sort(vec2.begin(), vec2.end());
    std::vector<uint64_t> v = it->second;
    std::sort(v.begin(), v.end());
    EXPECT_EQ(lift(vec2, size), v.size());
    for (uint64_t k = 0; k < lift(vec2, size); k++) {
      EXPECT_EQ(vec2[k], v[k]);
    }
  }
  table.deinitialize();
  vec.deinitialize();
}

TEST(Vector, EdgelistVectorOfVectors) {
  pando::Vector<pando::Vector<galois::WMDEdge>> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  uint64_t size = 2000;
  galois::HashTable<uint64_t, uint64_t> table;
  PANDO_CHECK(table.initialize(8));
  uint64_t result = 0;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> distribution(0, size);
  std::unordered_map<uint64_t, std::vector<uint64_t>> map;

  // Creates a vector of vectors of edges: src->dst
  for (uint64_t i = 0; i < size * 4; i++) {
    int src = distribution(gen);
    int dst = distribution(gen);
    if (map.find(src) == map.end()) {
      std::vector<uint64_t> v;
      v.push_back(dst);
      map[src] = v;
    } else {
      map[src].push_back(dst);
    }

    if (fmap(table, get, src, result)) {
      pando::GlobalRef<pando::Vector<galois::WMDEdge>> vec1 = fmap(vec, get, result);
      pando::Vector<galois::WMDEdge> vec2 = vec1;
      galois::WMDEdge edge(src, dst, agile::TYPES::NONE, agile::TYPES::NONE, agile::TYPES::NONE);
      EXPECT_EQ(fmap(vec1, pushBack, edge), pando::Status::Success);
    } else {
      PANDO_CHECK(fmap(table, put, src, lift(vec, size)));
      pando::Vector<galois::WMDEdge> v;
      EXPECT_EQ(v.initialize(1), pando::Status::Success);
      galois::WMDEdge edge(src, dst, agile::TYPES::NONE, agile::TYPES::NONE, agile::TYPES::NONE);
      v[0] = edge;
      EXPECT_EQ(vec.pushBack(v), pando::Status::Success);
    }
  }

  // Validates the vectors
  for (auto it = map.begin(); it != map.end(); ++it) {
    EXPECT_TRUE(fmap(table, get, it->first, result));
    pando::GlobalRef<pando::Vector<galois::WMDEdge>> vec1 = fmap(vec, get, result);
    pando::Vector<galois::WMDEdge> vec2 = vec1;
    std::vector<uint64_t> v = it->second;
    EXPECT_EQ(lift(vec2, size), v.size());
    for (uint64_t k = 0; k < lift(vec2, size); k++) {
      galois::WMDEdge edge = vec2[k];
      bool found = false;
      for (auto it2 = v.begin(); it2 != v.end(); ++it2) {
        if (edge.src == it->first && edge.dst == *it2) {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
  }
  table.deinitialize();
  vec.deinitialize();
}
