// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(PerHost, Init) {
  galois::PerHost<std::uint64_t> ph;
  pando::Status err;

  EXPECT_EQ(ph.initialize(), pando::Status::Success);
  std::uint64_t i = 0;
  for (pando::GlobalRef<std::uint64_t> val : ph) {
    val = i;
    i++;
  }
  auto f = +[](galois::PerHost<std::uint64_t> ph, std::uint64_t i, pando::NotificationHandle done) {
    EXPECT_EQ(&ph.getLocal(), &ph.get(i));
    EXPECT_EQ(ph.getLocal(), ph.getCurrentNode());
    done.notify();
  };

  pando::NotificationArray dones;
  err = dones.init(ph.getNumHosts());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < ph.getNumHosts(); i++) {
    auto place =
        pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)}, pando::anyPod, pando::anyCore};
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
  for (std::uint64_t i = 0; i < ph.getNumHosts(); i++) {
    auto place =
        pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)}, pando::anyPod, pando::anyCore};
    err = pando::executeOn(place, f, ph, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  ph.deinitialize();
}

TEST(PerHost, DoAll) {
  galois::PerHost<std::uint64_t> ph;
  pando::Status err;

  EXPECT_EQ(ph.initialize(), pando::Status::Success);

  for (std::uint64_t i = 0; i < ph.getNumHosts(); i++) {
    ph.get(i) = 0xDEADBEEF;
  }

  auto g = +[](pando::GlobalRef<std::uint64_t> val) {
    val = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
  };

  err = galois::doAll(ph, g);

  EXPECT_EQ(err, pando::Status::Success);

  auto f = +[](galois::PerHost<std::uint64_t> ph, std::uint64_t i, pando::NotificationHandle done) {
    EXPECT_EQ(&ph.getLocal(), &ph.get(i));
    EXPECT_EQ(ph.getLocal(), ph.getCurrentNode());
    done.notify();
  };
  pando::NotificationArray dones;
  err = dones.init(ph.getNumHosts());
  EXPECT_EQ(err, pando::Status::Success);
  for (std::uint64_t i = 0; i < ph.getNumHosts(); i++) {
    auto place =
        pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)}, pando::anyPod, pando::anyCore};
    err = pando::executeOn(place, f, ph, i, dones.getHandle(i));
    EXPECT_EQ(err, pando::Status::Success);
  }
  dones.wait();

  ph.deinitialize();
}
