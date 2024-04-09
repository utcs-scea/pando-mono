// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/pando-rt.hpp>

galois::DistArray<uint64_t> getDistributedWorkArray(uint64_t workItemsPerHost) {
  galois::DistArray<uint64_t> work{};

  int16_t pxns = pando::getPlaceDims().node.id;
  pando::Vector<galois::PlaceType> vec;
  EXPECT_EQ(vec.initialize(pxns), pando::Status::Success);

  for (std::int16_t i = 0; i < pxns; i++) {
    vec[i] = galois::PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                               pando::MemoryType::Main};
  }
  EXPECT_EQ(work.initialize(vec.begin(), vec.end(), pxns * workItemsPerHost),
            pando::Status::Success);
  return work;
}

TEST(DistAccumulator, Init) {
  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
}

TEST(DistAccumulator, SingleHost) {
  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);
  sum.increment();
  sum.add(10);
  sum.decrement();
  sum.decrement();
  sum.subtract(7);
  EXPECT_EQ(sum.get(), 0);
  EXPECT_EQ(sum.reduce(), 2);
  EXPECT_EQ(sum.get(), 2);
  sum.reset();
  EXPECT_EQ(sum.get(), 0);
}

TEST(DistAccumulator, Distributed) {
  const uint64_t workItemsPerHost = 1000;
  const uint64_t pxns = pando::getPlaceDims().node.id;
  galois::DistArray<uint64_t> distributedWork = getDistributedWorkArray(workItemsPerHost);
  galois::DAccumulator<uint64_t> sum{};
  EXPECT_EQ(sum.initialize(), pando::Status::Success);
  EXPECT_EQ(sum.get(), 0);

  galois::doAll(
      sum, distributedWork, +[](galois::DAccumulator<uint64_t> sum, pando::GlobalRef<uint64_t>) {
        sum.increment();
      });
  galois::doAll(
      sum, distributedWork, +[](galois::DAccumulator<uint64_t> sum, pando::GlobalRef<uint64_t>) {
        sum.increment();
      });
  EXPECT_EQ(sum.get(), 0);
  EXPECT_EQ(sum.reduce(), workItemsPerHost * pxns * 2);
  EXPECT_EQ(sum.get(), workItemsPerHost * pxns * 2);
  sum.reset();
  EXPECT_EQ(sum.get(), 0);
}
