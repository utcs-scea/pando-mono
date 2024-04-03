// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/simple_lock.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(SimpleLock, TryLock) {
  auto test = [] {
    galois::SimpleLock mutex;
    EXPECT_EQ(mutex.initialize(), pando::Status::Success);
    const bool wasUnlocked = mutex.try_lock();
    EXPECT_TRUE(wasUnlocked);

    const bool isUnlocked = mutex.try_lock();
    EXPECT_FALSE(isUnlocked);

    mutex.unlock();
    mutex.deinitialize();

    return true;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, +test);
  EXPECT_TRUE(result.hasValue());
}

TEST(SimpleLock, SimpleLockUnlock) {
  auto test = [] {
    galois::SimpleLock mutex;
    EXPECT_EQ(mutex.initialize(), pando::Status::Success);
    mutex.lock();
    mutex.unlock();
    mutex.deinitialize();
    return true;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, +test);
  EXPECT_TRUE(result.hasValue());
}

TEST(SimpleLock, ActualLockUnlock) {
  pando::Vector<int64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  EXPECT_EQ(vec.size(), 0);

  galois::SimpleLock mutex;

  auto test = [](galois::SimpleLock mutex, uint64_t host_id) {
    mutex.lock();
    vec.push_back(host_id);
    mutex.unlock();
    return true;
  };

  std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  galois::doAll(mutex, numHosts, +test);

  std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  galois::SimpleLock mutex;
  EXPECT_EQ(mutex.initialize(), pando::Status::Success);
  pando::Array<uint64_t> array;
  EXPECT_EQ(array.initialize(0), pando::Status::Success);
  EXPECT_EQ(vec.size(), 0);
  auto func = +[](galois::SimpleLock mutex, pando::Vector<int64_t> vec, std::uint64_t goodVal) {
    ptr[pando::getCurrentPlace().node.id] = goodVal;
    gb.done();
    EXPECT_EQ(gb.wait(), pando::Status::Success);
  };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, gb, ptr, goodVal),
        pando::Status::Success);
  }
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(*ptr, goodVal);
  }
  gb.deinitialize();
}
