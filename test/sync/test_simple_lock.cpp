// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/global_barrier.hpp>
#include <pando-lib-galois/sync/simple_lock.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
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
    galois::SimpleLock mutex{};
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
  galois::SimpleLock mutex;
  EXPECT_EQ(mutex.initialize(), pando::Status::Success);
  pando::Array<int> array;
  EXPECT_EQ(array.initialize(10), pando::Status::Success);
  array.fill(0);

  galois::HostLocalStorage<std::uint64_t> hls{};
  auto tpl = galois::make_tpl(mutex, array);
  EXPECT_EQ(galois::doAll(
                tpl, hls,
                +[](decltype(tpl) tpl, pando::GlobalRef<std::uint64_t>) {
                  auto [mutex, array] = tpl;
                  mutex.lock();
                  for (int i = 0; i < 10; i++) {
                    if ((i + 1 + pando::getCurrentPlace().node.id) <= 10) {
                      array[i] = i + 1 + pando::getCurrentPlace().node.id;
                    } else {
                      array[i] = i - 9 + pando::getCurrentPlace().node.id;
                    }
                  }
                  mutex.unlock();
                }),
            pando::Status::Success);

  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += array[i];
  }
  EXPECT_EQ(sum, 55);

  array.deinitialize();
  mutex.deinitialize();
}
