// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/specific_storage.hpp>

TEST(PodLocalStorage, Init) {
  galois::PodLocalStorage<std::uint64_t> ph;
  pando::Status err;

  EXPECT_EQ(ph.initialize(), pando::Status::Success);
  std::uint64_t i = 0;
  for (pando::GlobalRef<std::uint64_t> val : ph) {
    val = i;
    i++;
  }
  auto f = +[](galois::PodLocalStorage<std::uint64_t> ph, std::uint64_t i,
               pando::NotificationHandle done) {
    EXPECT_EQ(&ph.getLocal(), &ph.get(i));
    done.notify();
  };

  pando::NotificationArray dones;
  err = dones.init(ph.getNumPods());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < ph.getNumPods(); i++) {
    auto place = ph.getPlaceFromPodIdx(i);
    err = pando::executeOn(place, f, ph, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  ph.deinitialize();

  EXPECT_EQ(ph.initialize(), pando::Status::Success);
  i = 0;
  for (pando::GlobalRef<std::uint64_t> val : ph) {
    val = i;
    i++;
  }

  dones.reset();
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < ph.getNumPods(); i++) {
    auto place = ph.getPlaceFromPodIdx(i);
    err = pando::executeOn(place, f, ph, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  ph.deinitialize();
}

TEST(PodLocalStorage, DoAll) {
  galois::PodLocalStorage<std::uint64_t> ph;
  pando::Status err;

  EXPECT_EQ(ph.initialize(), pando::Status::Success);
  EXPECT_TRUE(ph == ph);
  EXPECT_FALSE(ph != ph);

  for (std::uint64_t i = 0; i < ph.getNumPods(); i++) {
    ph.get(i) = 0xDEADBEEF;
  }

  auto g = +[](pando::GlobalRef<std::uint64_t> val) {
    val = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
  };

  err = galois::doAll(ph, g);

  EXPECT_EQ(err, pando::Status::Success);

  auto f = +[](galois::PodLocalStorage<std::uint64_t> ph, pando::NotificationHandle done) {
    EXPECT_EQ(ph.getLocal(), static_cast<std::uint64_t>(pando::getCurrentPlace().node.id));
    done.notify();
  };
  pando::NotificationArray dones;
  err = dones.init(ph.getNumPods());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < ph.getNumPods(); i++) {
    const auto place = ph.getPlaceFromPodIdx(i);
    err = pando::executeOn(place, f, ph, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  ph.deinitialize();
}

TEST(PodLocalStorage, copyToAllHosts) {
  const std::uint64_t SIZE = 100;
  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(pando::Status::Success, arr.initialize(100));
  for (std::uint64_t i = 0; i < SIZE; i++) {
    arr[i] = i;
  }
  auto hlsarr = PANDO_EXPECT_CHECK(galois::copyToAllPods(arr));
  for (pando::Array<std::uint64_t> tocheck : hlsarr) {
    EXPECT_EQ(tocheck.size(), SIZE);
    for (std::uint64_t i = 0; i < SIZE; i++) {
      EXPECT_EQ(tocheck.get(i), i);
    }
    tocheck.deinitialize();
  }
  hlsarr.deinitialize();
}
