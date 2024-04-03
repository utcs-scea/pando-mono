// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/global_barrier.hpp>
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
  auto dims = pando::getPlaceDims();
  galois::GlobalBarrier gb;
  EXPECT_EQ(gb.initialize(dims.node.id), pando::Status::Success);
  galois::SimpleLock mutex;
  EXPECT_EQ(mutex.initialize(), pando::Status::Success);
  pando::Array<int> array;
  EXPECT_EQ(array.initialize(10), pando::Status::Success);
  array.fill(0);

  auto func = +[](galois::GlobalBarrier gb, galois::SimpleLock mutex, pando::Array<int> array) {
    mutex.lock();
    for (int i = 0; i < 10; i++) {
      if ((i + 1 + pando::getCurrentPlace().node.id) <= 10) {
        array[i] = i + 1 + pando::getCurrentPlace().node.id;
      } else {
        array[i] = i - 9 + pando::getCurrentPlace().node.id;
      }
    }
    // for (int i = 0; i < 10; i++) {
    //   std::cout << array[i] << " ";
    // }
    // std::cout << std::endl;
    mutex.unlock();
    gb.done();
  };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, gb, mutex, array),
        pando::Status::Success);
  }

  EXPECT_EQ(gb.wait(), pando::Status::Success);
  for (int i = 0; i < 10; i++) {
    std::cout << array[i] << " ";
  }
  std::cout << std::endl;
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += array[i];
  }
  EXPECT_EQ(sum, 55);

  gb.deinitialize();
  array.deinitialize();
}
