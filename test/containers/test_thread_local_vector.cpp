// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <random>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/thread_local_vector.hpp>
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

/**
uint64_t getHostThreads() {
  uint64_t x = pando::getPlaceDims().core.x;
  uint64_t y = pando::getPlaceDims().core.y;
  uint64_t threads = pando::getThreadDims().id;
  return x * y * threads;
}
*/

struct State {
  State() = default;
  State(galois::WaitGroup::HandleType f, galois::DAccumulator<uint64_t> s) : first(f), second(s) {}

  galois::WaitGroup::HandleType first;
  galois::DAccumulator<uint64_t> second;
};

} // namespace

TEST(ThreadLocalVector, Init) {
  galois::ThreadLocalVector<uint64_t> perThreadVec{};
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);
  pando::Vector<uint64_t> work;
  EXPECT_EQ(work.initialize(1), pando::Status::Success);
  work[0] = 9801;
  galois::doAll(
      perThreadVec, work, +[](galois::ThreadLocalVector<uint64_t> perThreadVec, uint64_t x) {
        EXPECT_GE(pando::getCurrentThread().id, 0);
        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);
        pando::Vector<uint64_t> localVec = perThreadVec.getLocalRef();
        EXPECT_EQ(localVec.size(), 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), 1);

  std::uint64_t elts = 0;
  for (pando::Vector<uint64_t> vec : perThreadVec) {
    elts += vec.size();
  }
  EXPECT_EQ(elts, 1);

  auto hca = PANDO_EXPECT_CHECK(perThreadVec.hostCachedFlatten());
  EXPECT_EQ(hca.size(), 1);
  uint64_t val = hca[0];
  EXPECT_EQ(val, 9801);

  hca.deinitialize();
  work.deinitialize();
  perThreadVec.deinitialize();
}

TEST(ThreadLocalVector, Parallel) {
  galois::ThreadLocalVector<uint64_t> perThreadVec{};
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);

  static const uint64_t workItems = 1000;
  pando::Vector<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  galois::doAll(
      perThreadVec, work, +[](galois::ThreadLocalVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getLocalRef();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getLocalRef();
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  uint64_t elts = 0;
  for (uint64_t i = 0; i < perThreadVec.size(); i++) {
    pando::Vector<uint64_t> vec = perThreadVec[i];
    elts += vec.size();
    for (uint64_t i = 0; i < vec.size(); i++) {
      EXPECT_LT(vec[i], workItems);
    }
  }
  EXPECT_EQ(elts, workItems);
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  galois::HostCachedArray<uint64_t> hca = PANDO_EXPECT_CHECK(perThreadVec.hostCachedFlatten());
  EXPECT_EQ(hca.size(), workItems);

  hca.deinitialize();
  work.deinitialize();
  perThreadVec.deinitialize();
}

TEST(ThreadLocalVector, DoAll) {
  galois::ThreadLocalVector<uint64_t> perThreadVec;
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);

  static const uint64_t workItems = 100;
  galois::DistArray<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  for (uint64_t i = 0; i < workItems; i++) {
    work[i] = i;
  }

  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      perThreadVec, work, +[](galois::ThreadLocalVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getLocalRef();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getLocalRef();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);
  const std::uint64_t size = perThreadVec.sizeAll();

  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(size, perThreadVec.sizeAll());

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

  galois::HostCachedArray<uint64_t> hca = PANDO_EXPECT_CHECK(perThreadVec.hostCachedFlatten());
  EXPECT_EQ(hca.size(), workItems);
  uint64_t copy_sum = 0;
  for (uint64_t elt : hca) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  hca.deinitialize();
  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.deinitialize();
}

TEST(ThreadLocalVector, HostLocalStorageVector) {
  constexpr std::uint64_t size = 32;
  pando::Status err;

  galois::ThreadLocalVector<std::uint64_t> ptv;
  err = ptv.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostLocalStorage<std::uint64_t> phu{};

  galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
      ptv, phu, +[](galois::ThreadLocalVector<std::uint64_t> ptv, std::uint64_t) {
        galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
            ptv, galois::IotaRange(0, size),
            +[](galois::ThreadLocalVector<std::uint64_t> ptv, std::uint64_t i) {
              pando::Status err;
              err = ptv.pushBack(i);
              EXPECT_EQ(err, pando::Status::Success);
            });
      });

  galois::HostLocalStorage<pando::Vector<std::uint64_t>> phv;
  PANDO_CHECK(phv.initialize());
  for (auto vecRef : phv) {
    EXPECT_EQ(fmap(vecRef, initialize, 0), pando::Status::Success);
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
}

TEST(ThreadLocalVector, Clear) {
  constexpr std::uint64_t size = 32;
  pando::Status err;

  galois::ThreadLocalVector<std::uint64_t> ptv;
  err = ptv.initialize();
  EXPECT_EQ(err, pando::Status::Success);

  galois::HostLocalStorage<std::uint64_t> phu{};

  galois::doAll(
      ptv, phu, +[](galois::ThreadLocalVector<std::uint64_t> ptv, std::uint64_t) {
        galois::doAll(
            ptv, galois::IotaRange(0, size),
            +[](galois::ThreadLocalVector<std::uint64_t> ptv, std::uint64_t i) {
              pando::Status err;
              err = ptv.pushBack(i);
              EXPECT_EQ(err, pando::Status::Success);
            });
      });

  galois::DAccumulator<std::uint64_t> accum{};
  EXPECT_EQ(accum.initialize(), pando::Status::Success);

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

TEST(ThreadLocalVector, ClearCompute) {
  galois::ThreadLocalVector<uint64_t> perThreadVec;
  EXPECT_EQ(perThreadVec.initialize(), pando::Status::Success);

  static uint64_t workItems = 100;
  galois::DistArray<uint64_t> work;
  EXPECT_EQ(work.initialize(workItems), pando::Status::Success);
  for (uint64_t i = 0; i < workItems; i++) {
    work[i] = i;
  }

  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      perThreadVec, work, +[](galois::ThreadLocalVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getLocalRef();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getLocalRef();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  const std::uint64_t sizeAll0 = perThreadVec.sizeAll();
  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(sizeAll0, perThreadVec.sizeAll());

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

  galois::HostCachedArray<uint64_t> hca = PANDO_EXPECT_CHECK(perThreadVec.hostCachedFlatten());
  EXPECT_EQ(hca.size(), workItems);
  uint64_t copy_sum = 0;
  for (uint64_t elt : hca) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  hca.deinitialize();
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
      perThreadVec, work, +[](galois::ThreadLocalVector<uint64_t>& perThreadVec, uint64_t x) {
        uint64_t originalID = pando::getCurrentThread().id;
        EXPECT_GE(originalID, 0);
        EXPECT_LT(originalID, pando::getThreadDims().id);
        pando::Vector<uint64_t> staleVec = perThreadVec.getLocalRef();

        EXPECT_EQ(perThreadVec.pushBack(x), pando::Status::Success);

        pando::Vector<uint64_t> localVec = perThreadVec.getLocalRef();
        EXPECT_EQ(pando::localityOf(localVec.data()).node.id, pando::getCurrentPlace().node.id);
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

  const std::uint64_t sizeAll1 = perThreadVec.sizeAll();
  EXPECT_EQ(perThreadVec.computeIndices(), pando::Status::Success);
  EXPECT_EQ(sizeAll1, perThreadVec.sizeAll());

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

  hca = PANDO_EXPECT_CHECK(perThreadVec.hostCachedFlatten());
  EXPECT_EQ(hca.size(), workItems);
  copy_sum = 0;
  for (uint64_t elt : hca) {
    copy_sum += elt;
  }
  EXPECT_EQ(copy_sum, ((workItems - 1) + 0) * (workItems / 2));

  hca.deinitialize();
  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.deinitialize();
}
