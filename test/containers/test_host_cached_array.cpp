// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <algorithm>

#include <pando-lib-galois/containers/host_cached_array.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(HostCachedArray, Empty) {
  galois::HostCachedArray<std::uint64_t> array;
  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  for (auto ref : sizes) {
    ref = 0;
  }
  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);
  EXPECT_EQ(array.size(), 0);
  EXPECT_TRUE(array.empty());
  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, ExecuteOn) {
  constexpr std::uint64_t goodVal = 0xDEADBEEF;

  constexpr std::uint64_t size = 5;
  const std::uint64_t nodes = pando::getPlaceDims().node.id;

  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  // create array
  galois::HostCachedArray<std::uint64_t> array;

  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    array[i] = 0xDEADBEEF;
  }

  pando::Status status;
  auto func = +[](pando::NotificationHandle done, std::uint64_t goodVal,
                  galois::HostCachedArray<std::uint64_t> hca) {
    for (auto curr : hca) {
      EXPECT_EQ(curr, goodVal);
    }
    done.notify();
  };
  pando::Notification notif;
  EXPECT_EQ(notif.init(), pando::Status::Success);
  status = pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func,
                            notif.getHandle(), goodVal, array);
  EXPECT_EQ(status, pando::Status::Success);
  notif.wait();

  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, Initialize) {
  constexpr std::uint64_t size = 10;

  const std::uint64_t nodes = pando::getPlaceDims().node.id;
  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  galois::HostCachedArray<std::uint64_t> array;
  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);
  EXPECT_EQ(array.size(), size * nodes);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    std::int16_t nodeIdx = i / size;
    EXPECT_EQ(pando::localityOf(&array[i]).node.id, nodeIdx);
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    EXPECT_EQ(array[i], i);
  }

  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, Swap) {
  const std::uint64_t size0 = 10;
  const std::uint64_t size1 = 16;
  const std::uint64_t nodes = pando::getPlaceDims().node.id;
  pando::Array<std::uint64_t> sizes0;
  pando::Array<std::uint64_t> sizes1;

  EXPECT_EQ(sizes0.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes0) {
    ref = size0;
  }

  EXPECT_EQ(sizes1.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes1) {
    ref = size1;
  }

  galois::HostCachedArray<std::uint64_t> array0;
  EXPECT_EQ(array0.initialize(sizes0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size0 * nodes; i++) {
    array0[i] = i;
  }

  galois::HostCachedArray<std::uint64_t> array1;
  EXPECT_EQ(array1.initialize(sizes1), pando::Status::Success);
  for (std::uint64_t i = 0; i < size1 * nodes; i++) {
    array1[i] = i + (size0 * nodes);
  }

  std::swap(array0, array1);

  for (std::uint64_t i = 0; i < size1 * nodes; i++) {
    EXPECT_EQ(array0[i], i + (size0 * nodes));
  }

  for (std::uint64_t i = 0; i < size0 * nodes; i++) {
    EXPECT_EQ(array1[i], i);
  }

  sizes0.deinitialize();
  sizes1.deinitialize();
  array0.deinitialize();
  array1.deinitialize();
}

TEST(HostCachedArray, Iterator) {
  const std::uint64_t size = 25;

  // create array
  galois::HostCachedArray<std::uint64_t> array;

  const std::uint64_t nodes = pando::getPlaceDims().node.id;
  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    array[i] = i;
  }
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  std::uint64_t i = 0;
  for (std::uint64_t val : array) {
    EXPECT_EQ(val, i);
    i++;
  }

  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, IteratorManual) {
  const std::uint64_t size = 25;

  // create array
  galois::HostCachedArray<std::uint64_t> array;

  const std::uint64_t nodes = pando::getPlaceDims().node.id;
  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    EXPECT_EQ(array[i], i);
  }

  std::uint64_t i = 0;
  for (auto curr = array.begin(); curr != array.end(); curr++) {
    EXPECT_EQ(*curr, i);
    i++;
  }

  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, ReverseIterator) {
  const std::uint64_t size = 25;

  // create array
  galois::HostCachedArray<std::uint64_t> array;

  const std::uint64_t nodes = pando::getPlaceDims().node.id;
  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    EXPECT_EQ(array[i], i);
  }

  std::uint64_t i = array.size() - 1;
  for (auto curr = array.rbegin(); curr != array.rend(); curr++) {
    EXPECT_EQ(*curr, i);
    i--;
  }

  array.deinitialize();
  sizes.deinitialize();
}

TEST(HostCachedArray, IteratorExecuteOn) {
  using It = galois::HostCachedArrayIterator<std::uint64_t>;
  constexpr std::uint64_t goodVal = 0xDEADBEEF;

  constexpr std::uint64_t size = 5;
  const std::uint64_t nodes = pando::getPlaceDims().node.id;

  pando::Array<std::uint64_t> sizes;
  EXPECT_EQ(sizes.initialize(nodes), pando::Status::Success);
  for (auto ref : sizes) {
    ref = size;
  }

  // create array
  galois::HostCachedArray<std::uint64_t> array;

  EXPECT_EQ(array.initialize(sizes), pando::Status::Success);

  for (std::uint64_t i = 0; i < size * nodes; i++) {
    array[i] = 0xDEADBEEF;
  }

  pando::Status status;
  auto func = +[](pando::NotificationHandle done, std::uint64_t goodVal, It begin, It end) {
    for (auto curr = begin; curr != end; curr++) {
      EXPECT_EQ(*curr, goodVal);
    }
    done.notify();
  };
  pando::Notification notif;
  EXPECT_EQ(notif.init(), pando::Status::Success);
  status = pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func,
                            notif.getHandle(), goodVal, array.begin(), array.end());
  EXPECT_EQ(status, pando::Status::Success);
  notif.wait();

  array.deinitialize();
  sizes.deinitialize();
}
