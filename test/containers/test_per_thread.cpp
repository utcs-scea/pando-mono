// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
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
  pando::GlobalPtr<pando::Vector<uint64_t>> remoteVec = getGlobalObject<pando::Vector<uint64_t>>();
  EXPECT_EQ(perThreadVec.assign(remoteVec), pando::Status::Success);
  pando::Vector<uint64_t> copy = *remoteVec;
  EXPECT_EQ(copy.size(), 1);

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
  pando::GlobalPtr<pando::Vector<uint64_t>> remoteVec = getGlobalObject<pando::Vector<uint64_t>>();
  EXPECT_EQ(perThreadVec.assign(remoteVec), pando::Status::Success);
  pando::Vector<uint64_t> copy = *remoteVec;
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

  galois::DAccumulator<uint64_t> sum;
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
        EXPECT_GT(localVec.size(), 0);
        EXPECT_LT(localVec.size(), workItems);
        EXPECT_EQ(localVec.size(), staleVec.size() + 1);
      });
  EXPECT_EQ(perThreadVec.sizeAll(), workItems);

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

  sum.deinitialize();
  work.deinitialize();
  wg.deinitialize();
  perThreadVec.deinitialize();
}
