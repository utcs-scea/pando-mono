// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <pando-rt/export.h>

#include <variant>

#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/sync/notification.hpp>

TEST(doALL, SimpleCopy) {
  pando::Status err;
  pando::Notification necessary;
  err = necessary.init();
  EXPECT_EQ(err, pando::Status::Success);
  auto f = +[](pando::Notification::HandleType done) {
    auto const size = 10;
    pando::Vector<std::uint64_t> src;
    pando::Status err;

    err = src.initialize(size);
    EXPECT_EQ(err, pando::Status::Success);
    for (std::uint64_t i = 0; i < src.size(); i++) {
      src[i] = i;
    }
    auto plusOne = +[](pando::GlobalRef<std::uint64_t> v) {
      v = v + 1;
    };
    galois::doAll(src, plusOne);
    EXPECT_EQ(src.size(), size);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(src[i], i + 1);
    }
    auto plusN = +[](std::uint64_t n, pando::GlobalRef<std::uint64_t> v) {
      v = v + n;
    };
    const std::uint64_t N = 10;

    galois::doAll(N, src, plusN);

    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(src[i], i + 11);
    }

    done.notify();
  };

  err = pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                         necessary.getHandle());
  EXPECT_EQ(err, pando::Status::Success);
  necessary.wait();
}

TEST(doALL, NestedInit) {
  constexpr std::uint64_t SIZE = 10;
  constexpr std::uint64_t VALUE = 0xDEADBEEF;
  pando::Array<pando::Array<pando::Array<std::uint64_t>>> distArray;
  EXPECT_EQ(distArray.initialize(SIZE), pando::Status::Success);

  galois::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);

  auto f = +[](galois::WaitGroup::HandleType wgh,
               pando::GlobalRef<pando::Array<pando::Array<std::uint64_t>>> arrArrRef) {
    auto g = +[](galois::WaitGroup::HandleType wgh,
                 pando::GlobalRef<pando::Array<std::uint64_t>> arrRef) {
      auto h = +[](pando::GlobalRef<std::uint64_t> ref) {
        constexpr std::uint64_t VALUE = 0xDEADBEEF;
        ref = VALUE;
      };
      constexpr std::uint64_t SIZE = 10;
      pando::Array<std::uint64_t> arr;
      EXPECT_EQ(arr.initialize(SIZE), pando::Status::Success);
      galois::doAll(wgh, arr, h);
      arrRef = arr;
    };
    constexpr std::uint64_t SIZE = 10;
    pando::Array<pando::Array<std::uint64_t>> arrArr;
    EXPECT_EQ(arrArr.initialize(SIZE), pando::Status::Success);
    galois::doAll(wgh, wgh, arrArr, g);
    arrArrRef = arrArr;
  };
  EXPECT_EQ(galois::doAll(wg.getHandle(), wg.getHandle(), distArray, f), pando::Status::Success);
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  wg.deinitialize();

  for (pando::Array<pando::Array<std::uint64_t>> arrArr : distArray) {
    for (pando::Array<std::uint64_t> arr : arrArr) {
      for (std::uint64_t val : arr) {
        EXPECT_EQ(val, VALUE);
      }
      arr.deinitialize();
    }
    arrArr.deinitialize();
  }
  distArray.deinitialize();
}

TEST(doALL, CustomLocality) {
  constexpr std::uint64_t SIZE = 25;
  bool extraState = true;
  pando::Array<uint64_t> arr;
  EXPECT_EQ(arr.initialize(SIZE), pando::Status::Success);
  for (uint64_t i = 0; i < arr.size(); i++) {
    arr[i] = i;
  }
  galois::doAll(
      extraState, arr,
      +[](bool extraState, pando::GlobalRef<uint64_t> elt) {
        if (extraState) {
          elt = pando::getCurrentPlace().node.id;
        }
      },
      +[](bool extraState, uint64_t elt) {
        if (extraState) {
          return pando::Place{pando::NodeIndex{(int16_t)(elt % pando::getPlaceDims().node.id)},
                              pando::anyPod, pando::anyCore};
        } else {
          return pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};
        }
      });
  for (uint64_t i = 0; i < arr.size(); i++) {
    EXPECT_EQ(arr[i], i % pando::getPlaceDims().node.id);
  }
  arr.deinitialize();
}

TEST(onEach, VerifyLocality) {
  galois::DAccumulator<uint64_t> loops{};
  EXPECT_EQ(loops.initialize(), pando::Status::Success);
  galois::onEach(
      loops, +[](galois::DAccumulator<uint64_t>& loops, uint64_t threadID, uint64_t ts) {
        loops.add(threadID);
        uint64_t coreY = pando::getPlaceDims().core.y;
        uint64_t cores = pando::getPlaceDims().core.x * coreY;
        uint64_t threads = pando::getThreadDims().id;
        uint64_t hosts = pando::getPlaceDims().node.id;
        uint64_t totalThreads = hosts * cores * threads;
        EXPECT_EQ(ts, totalThreads);
        EXPECT_EQ(threadID / cores / threads, (uint64_t)pando::getCurrentPlace().node.id);
      });
  uint64_t coreY = pando::getPlaceDims().core.y;
  uint64_t cores = pando::getPlaceDims().core.x * coreY;
  uint64_t threads = pando::getThreadDims().id;
  uint64_t hosts = pando::getPlaceDims().node.id;
  uint64_t totalThreads = hosts * cores * threads;
  EXPECT_EQ(loops.reduce(), (totalThreads - 1 + 0) * (totalThreads) / 2);
}

TEST(doAll, EvenlyPartition) {
  // deliberately prime
  static const uint64_t workItems = 71;
  galois::DAccumulator<uint64_t> loops{};
  EXPECT_EQ(loops.initialize(), pando::Status::Success);
  galois::doAllEvenlyPartition(
      loops, workItems,
      +[](galois::DAccumulator<uint64_t>& loops, uint64_t workItem, uint64_t workItemsLocal) {
        loops.add(workItem);
        uint64_t hosts = pando::getPlaceDims().node.id;
        uint64_t workPerHost = workItemsLocal / hosts;
        if (workPerHost == 0) {
          workPerHost = 1;
        }
        uint64_t expectedHost = workItem / workPerHost;
        if (expectedHost >= hosts) {
          expectedHost = hosts - 1;
          EXPECT_LT(workPerHost * hosts, workItemsLocal);
        }
        EXPECT_EQ(expectedHost, (uint64_t)pando::getCurrentPlace().node.id);
      });
  EXPECT_EQ(loops.reduce(), (workItems - 1 + 0) * (workItems) / 2);
}
