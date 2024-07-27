// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/thread_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/specific_storage.hpp>

TEST(ThreadLocalStorage, DimensionalManipulation) {
  galois::ThreadLocalStorage<std::uint64_t> tls;
  for (std::uint64_t i = 0; i < tls.size(); i++) {
    auto [place, thread] = galois::getPlaceFromThreadIdx(i);
    EXPECT_EQ(galois::getThreadIdxFromPlace(place, thread), i);
  }
}

TEST(ThreadLocalStorage, Init) {
  galois::ThreadLocalStorage<std::uint64_t> tls;
  pando::Status err;

  EXPECT_EQ(tls.initialize(), pando::Status::Success);
  std::uint64_t i = 0;
  for (pando::GlobalRef<std::uint64_t> val : tls) {
    val = i;
    i++;
  }
  auto f = +[](galois::ThreadLocalStorage<std::uint64_t> tls, std::uint64_t i,
               pando::NotificationHandle done) {
    EXPECT_EQ(pando::localityOf(tls.getLocal()), pando::localityOf(tls.get(i)));
    done.notify();
  };

  pando::NotificationArray dones;
  err = dones.init(galois::getNumThreads());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < tls.size(); i++) {
    auto [place, _thread] = galois::getPlaceFromThreadIdx(i);
    err = pando::executeOn(place, f, tls, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  tls.deinitialize();

  EXPECT_EQ(tls.initialize(), pando::Status::Success);
  i = 0;
  for (pando::GlobalRef<std::uint64_t> val : tls) {
    val = i;
    i++;
  }

  dones.reset();
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < tls.size(); i++) {
    auto [place, _thread] = galois::getPlaceFromThreadIdx(i);
    err = pando::executeOn(place, f, tls, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  tls.deinitialize();
}

TEST(ThreadLocalStorage, DoAll) {
  galois::ThreadLocalStorage<std::uint64_t> tls;
  pando::Status err;

  EXPECT_EQ(tls.initialize(), pando::Status::Success);
  EXPECT_TRUE(tls == tls);
  EXPECT_FALSE(tls != tls);

  for (std::uint64_t i = 0; i < galois::getNumThreads(); i++) {
    tls[i] = 0xDEADBEEF;
  }

  auto g = +[](pando::GlobalRef<std::uint64_t> val) {
    auto place =
        pando::Place(pando::getCurrentPlace().node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0});
    const std::uint64_t threadIdx = galois::getThreadIdxFromPlace(place, pando::ThreadIndex(0));
    val = threadIdx;
  };

  err = galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(tls, g);

  EXPECT_EQ(err, pando::Status::Success);

  auto f = +[](galois::ThreadLocalStorage<std::uint64_t> tls, pando::NotificationHandle done) {
    auto place =
        pando::Place(pando::getCurrentPlace().node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0});
    const std::uint64_t threadIdx = galois::getThreadIdxFromPlace(place, pando::ThreadIndex(0));
    EXPECT_EQ(tls.getLocalRef(), threadIdx);
    done.notify();
  };
  pando::NotificationArray dones;
  err = dones.init(galois::getNumThreads());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < galois::getNumThreads(); i++) {
    const auto [place, _thread] = galois::getPlaceFromThreadIdx(i);
    err = pando::executeOn(place, f, tls, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  tls.deinitialize();
}

TEST(ThreadLocalStorage, copyToAllThreads) {
  const std::uint64_t SIZE = 10;
  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(pando::Status::Success, arr.initialize(SIZE));
  for (std::uint64_t i = 0; i < SIZE; i++) {
    arr[i] = i;
  }
  auto tlsarr = PANDO_EXPECT_CHECK(galois::copyToAllThreads(arr));
  for (pando::Array<std::uint64_t> tocheck : tlsarr) {
    EXPECT_EQ(tocheck.size(), SIZE);
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_EQ(tocheck.get(i), i);
    }
    tocheck.deinitialize();
  }
  tlsarr.deinitialize();
}
