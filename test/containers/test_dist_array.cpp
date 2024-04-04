// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <algorithm>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

struct PlaceType {
  pando::Place place;
  pando::MemoryType memType;
};

TEST(DistArray, Empty) {
  // This paradigm is necessary because the CP system is not part of the PGAS system
  pando::Notification necessary;
  EXPECT_EQ(necessary.init(), pando::Status::Success);

  auto f = +[](pando::Notification::HandleType done) {
    galois::DistArray<std::uint64_t> array;
    pando::Vector<PlaceType> vec;
    EXPECT_EQ(vec.initialize(0), pando::Status::Success);
    EXPECT_EQ(array.initialize(vec.begin(), vec.end(), 0), pando::Status::Success);
    EXPECT_EQ(array.size(), 0);
    array.deinitialize();
    vec.deinitialize();

    done.notify();
  };

  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             necessary.getHandle()),
            pando::Status::Success);
  necessary.wait();
}

TEST(DistArray, Initialize) {
  const std::uint64_t size = 10;
  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(size), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  galois::DistArray<std::uint64_t> array;

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);
  EXPECT_EQ(array.size(), size);

  for (std::uint64_t i = 0; i < size; i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    EXPECT_EQ(pando::localityOf(&array[i]).node.id, nodeIdx);
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, Swap) {
  const std::uint64_t size0 = 10;
  const std::uint64_t size1 = 16;
  pando::Vector<PlaceType> vec0;
  EXPECT_EQ(vec0.initialize(size0), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size0); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec0[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                        pando::MemoryType::Main};
  }

  pando::Vector<PlaceType> vec1;
  EXPECT_EQ(vec1.initialize(size1), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size1); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec1[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                        pando::MemoryType::Main};
  }

  galois::DistArray<std::uint64_t> array0;
  EXPECT_EQ(array0.initialize(vec0.begin(), vec0.end(), size0), pando::Status::Success);

  for (std::uint64_t i = 0; i < size0; i++) {
    array0[i] = i;
  }

  galois::DistArray<std::uint64_t> array1;
  EXPECT_EQ(array1.initialize(vec1.begin(), vec1.end(), size1), pando::Status::Success);

  for (std::uint64_t i = 0; i < size1; i++) {
    array1[i] = i + size0;
  }

  std::swap(array0, array1);

  for (std::uint64_t i = 0; i < size1; i++) {
    EXPECT_EQ(array0[i], i + size0);
  }

  for (std::uint64_t i = 0; i < size0; i++) {
    EXPECT_EQ(array1[i], i);
  }

  vec0.deinitialize();
  vec1.deinitialize();
  array0.deinitialize();
  array1.deinitialize();
}

TEST(DistArray, Iterator) {
  const std::uint64_t size = 100;

  // create array
  galois::DistArray<std::uint64_t> array;

  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
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
  vec.deinitialize();
}

TEST(DistArray, IteratorManual) {
  const std::uint64_t size = 100;

  // create array
  galois::DistArray<std::uint64_t> array;

  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
    array[i] = i;
  }
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  std::uint64_t i = 0;
  for (auto curr = array.begin(); curr != array.end(); curr++) {
    EXPECT_EQ(*curr, i);
    i++;
  }

  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, ReverseIterator) {
  const std::uint64_t size = 100;

  // create array
  galois::DistArray<std::uint64_t> array;

  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
    array[i] = i;
  }
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  std::uint64_t i = array.size() - 1;
  for (auto curr = array.rbegin(); curr != array.rend(); curr++) {
    EXPECT_EQ(*curr, i);
    i--;
  }

  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, IteratorExecuteOn) {
  using DI = galois::DAIterator<std::uint64_t>;
  static_assert(std::is_trivially_copyable<DI>::value);
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t goodVal = 0xDEADBEEF;

  // create array
  galois::DistArray<std::uint64_t> array;

  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
    array[i] = 0xDEADBEEF;
  }

  pando::Status status;
  auto func = +[](pando::NotificationHandle done, std::uint64_t goodVal, DI begin, DI end) {
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
  vec.deinitialize();
}

TEST(DistArray, Sort) {
  const std::uint64_t size = 10;
  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(size), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }
  galois::DistArray<std::uint64_t> array;

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);
  EXPECT_EQ(array.size(), size);
  std::uint64_t i = 0;
  for (pando::GlobalRef<std::uint64_t> ref : array) {
    ref = size - i - 1;
    i++;
  }
  std::sort(array.begin(), array.end());
  i = 0;
  for (std::uint64_t val : array) {
    EXPECT_EQ(val, i);
    i++;
  }
  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, SortPair) {
  const std::uint64_t size = 10;
  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(size), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }
  galois::DistArray<galois::Pair<std::uint64_t, std::uint64_t>> array;

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size * size), pando::Status::Success);
  EXPECT_EQ(array.size(), size * size);
  for (std::uint64_t i = 0; i < size; i++) {
    for (std::uint64_t j = 0; j < size; j++) {
      array[i * size + j] = galois::Pair<std::uint64_t, std::uint64_t>{size - i - 1, size - j - 1};
    }
  }

  std::sort(array.begin(), array.end());
  for (std::uint64_t i = 0; i < size; i++) {
    for (std::uint64_t j = 0; j < size; j++) {
      galois::Pair<std::uint64_t, std::uint64_t> totest = array[i * size + j];
      galois::Pair<std::uint64_t, std::uint64_t> expect =
          galois::Pair<std::uint64_t, std::uint64_t>{i, j};
      EXPECT_EQ(totest, expect);
    }
  }
  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, From) {
  const std::uint64_t size = 103;
  const std::uint64_t oversized = 257;
  pando::Vector<uint64_t> vec;
  EXPECT_EQ(vec.initialize(oversized), pando::Status::Success);
  for (uint64_t i = 0; i < size; i++) {
    vec[i] = i;
  }
  galois::DistArray<std::uint64_t> array;
  EXPECT_EQ(array.from(vec, size), pando::Status::Success);
  EXPECT_EQ(array.size(), size);
  for (uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }
  array.deinitialize();
  vec.deinitialize();
}

TEST(DistArray, DISABLED_DistributedSort) {
  const std::uint64_t size = 103;
  galois::DistArray<std::uint64_t> array;
  EXPECT_EQ(array.initialize(size), pando::Status::Success);
  for (uint64_t i = 0; i < array.size(); i++) {
    array[i] = i;
  }
  std::random_shuffle(array.begin(), array.end());
  EXPECT_EQ(array.sort(), pando::Status::Success);
  for (uint64_t i = 1; i < array.size(); i++) {
    uint64_t val1 = array[i - 1];
    uint64_t val2 = array[i];
    EXPECT_LE(val1, val2);
    EXPECT_EQ(val1, i - 1);
    EXPECT_EQ(val2, i);
  }
  array.deinitialize();
}

TEST(DistArray, DeinitializeWgh) {
  const std::uint64_t size = 10;
  pando::Vector<PlaceType> vec;
  EXPECT_EQ(vec.initialize(size), pando::Status::Success);
  for (std::int16_t i = 0; i < static_cast<std::int16_t>(size); i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    vec[i] = PlaceType{pando::Place{pando::NodeIndex{nodeIdx}, pando::anyPod, pando::anyCore},
                       pando::MemoryType::Main};
  }

  galois::DistArray<std::uint64_t> array;

  EXPECT_EQ(array.initialize(vec.begin(), vec.end(), size), pando::Status::Success);
  EXPECT_EQ(array.size(), size);

  for (std::uint64_t i = 0; i < size; i++) {
    std::int16_t nodeIdx = i % pando::getPlaceDims().node.id;
    EXPECT_EQ(pando::localityOf(&array[i]).node.id, nodeIdx);
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  pando::WaitGroup wg;
  EXPECT_EQ(wg.initialize(0), pando::Status::Success);
  array.deinitialize(wg.getHandle());
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  wg.deinitialize();
  vec.deinitialize();
}
