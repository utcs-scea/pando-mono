// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "gtest/gtest.h"

#include "pando-rt/sync/mutex.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"
#include "pando-rt/sync/notification.hpp"

TEST(Mutex, tryLock) {
  auto test = [] {
    pando::Mutex mutex;
    const bool wasUnlocked = mutex.try_lock();
    EXPECT_TRUE(wasUnlocked);

    const bool isUnlocked = mutex.try_lock();
    EXPECT_FALSE(isUnlocked);

    mutex.unlock();

    return true;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, +test);
  EXPECT_TRUE(result.hasValue());
}

TEST(Mutex, LockUnlock) {
  auto test = [] {
    pando::Mutex mutex;
    mutex.lock();
    mutex.unlock();
    return true;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, +test);
  EXPECT_TRUE(result.hasValue());
}
