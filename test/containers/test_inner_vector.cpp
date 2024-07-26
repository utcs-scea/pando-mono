// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2024. University of Texas at Austin. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/export.h"

#include <pando-lib-galois/containers/inner_vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>

TEST(InnerVector, Empty) {
  galois::InnerVector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(0), pando::Status::Success);
  EXPECT_TRUE(vector.empty());
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(vector.capacity(), 0);

  vector.deinitialize();
  EXPECT_EQ(vector.capacity(), 0);
  EXPECT_EQ(vector.size(), 0);
}

TEST(InnerVector, Initialize) {
  const std::uint64_t size = 10;

  galois::InnerVector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(size), pando::Status::Success);
  EXPECT_FALSE(vector.empty());
  EXPECT_EQ(vector.size(), size);
  EXPECT_EQ(vector.capacity(), size);

  for (std::uint64_t i = 0; i < size; i++) {
    vector[i] = i;
  }

  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vector[i], i);
  }

  vector.deinitialize();
  EXPECT_EQ(vector.capacity(), 0);
  EXPECT_EQ(vector.size(), 0);
}

TEST(InnerVector, PushBack) {
  const std::uint64_t size = 10;
  const std::uint64_t newCap = 16;

  galois::InnerVector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
    vector[i] = i;
  }

  EXPECT_EQ(vector.pushBack(size), pando::Status::Success);

  // This is only for power of 2 allocators
  EXPECT_EQ(vector.capacity(), newCap);
  EXPECT_FALSE(vector.empty());
  EXPECT_EQ(vector.size(), size + 1);

  for (std::uint64_t i = 0; i < size + 1; i++) {
    EXPECT_EQ(vector[i], i);
  }

  for (std::uint64_t i = size + 1; i < newCap; i++) {
    EXPECT_EQ(vector.pushBack(i), pando::Status::Success);

    EXPECT_EQ(vector.capacity(), newCap);
    EXPECT_FALSE(vector.empty());
    EXPECT_EQ(vector.size(), i + 1);
  }

  for (std::uint64_t i = 0; i < newCap; i++) {
    EXPECT_EQ(vector[i], i);
  }

  vector.deinitialize();
}

TEST(InnerVector, Clear) {
  const std::uint64_t size = 10;

  galois::InnerVector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(size), pando::Status::Success);

  for (std::uint64_t i = 0; i < size; i++) {
    vector[i] = i;
  }

  vector.clear();
  EXPECT_EQ(vector.size(), 0);
  EXPECT_TRUE(vector.empty());
  EXPECT_EQ(vector.capacity(), size);

  vector.deinitialize();
}

TEST(InnerVector, RemotePushBack) {
  auto f = +[](pando::Notification::HandleType done) {
    auto pushBackF = +[](pando::Notification::HandleType done,
                         pando::GlobalPtr<galois::InnerVector<std::uint64_t>> vectorPtr) {
      auto vector = static_cast<galois::InnerVector<std::uint64_t>>(*vectorPtr);
      EXPECT_EQ(vector.initialize(0), pando::Status::Success);
      EXPECT_EQ(vector.reserve(1), pando::Status::Success);
      EXPECT_EQ(vector.pushBack(1u), pando::Status::Success);
      EXPECT_EQ(vector.pushBack(2u), pando::Status::Success);
      EXPECT_EQ(vector.size(), 2);
      *vectorPtr = vector;
      done.notify();
    };

    galois::InnerVector<std::uint64_t> vector;
    pando::Notification innerNotification;
    EXPECT_EQ(innerNotification.init(), pando::Status::Success);
    EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                               pushBackF, innerNotification.getHandle(),
                               pando::GlobalPtr<galois::InnerVector<std::uint64_t>>(&vector)),
              pando::Status::Success);
    innerNotification.wait();

    EXPECT_EQ(vector.size(), 2u);
    EXPECT_EQ(vector[0], 1u);
    EXPECT_EQ(vector[1], 2u);

    vector.deinitialize();

    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             notification.getHandle()),
            pando::Status::Success);
  notification.wait();
}

TEST(InnerVector, StressCreateDestroy) {
  const std::uint64_t requests = 10;
  pando::NotificationArray notifications;
  EXPECT_EQ(notifications.init(requests), pando::Status::Success);
  for (std::uint64_t i = 0; i < requests; i++) {
    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore},
                  +[](pando::Notification::HandleType done) {
                    constexpr auto size = 1;
                    galois::InnerVector<std::uint64_t> vec;
                    EXPECT_EQ(vec.initialize(size), pando::Status::Success);
                    vec.deinitialize();
                    done.notify();
                  },
                  notifications.getHandle(i)),
              pando::Status::Success);
  }
  notifications.wait();
}

TEST(InnerVector, StressPushBack) {
  const std::uint64_t size = 8;
  const std::uint64_t finalSz = 1 << 6;

  galois::InnerVector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(size), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    vector[i] = i;
  }
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vector[i], i);
  }

  for (std::uint64_t currSz = size; currSz < finalSz; currSz++) {
    EXPECT_EQ(vector.pushBack(currSz), pando::Status::Success);
    EXPECT_EQ(vector.size(), currSz + 1);
    EXPECT_FALSE(vector.empty());
    EXPECT_GE(vector.capacity(), vector.size());

    for (std::uint64_t i = 0; i < currSz + 1; i++) {
      EXPECT_EQ(vector[i], i);
    }
  }

  vector.deinitialize();
}

TEST(InnerVector, MultiNodePushBack) {
  auto f = +[](pando::Notification::HandleType done) {
    const std::uint64_t size = 8;
    const std::uint64_t finalSz = 1 << 8;

    galois::InnerVector<std::uint64_t> vector;
    EXPECT_EQ(vector.initialize(size), pando::Status::Success);
    for (std::uint64_t i = 0; i < size; i++) {
      vector[i] = i;
    }
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(vector[i], i);
    }

    for (std::uint64_t currSz = size; currSz < finalSz; currSz++) {
      EXPECT_EQ(vector.pushBack(currSz), pando::Status::Success);
      EXPECT_EQ(vector.size(), currSz + 1);
      EXPECT_FALSE(vector.empty());
      EXPECT_GE(vector.capacity(), vector.size());

      for (std::uint64_t i = 0; i < currSz + 1; i++) {
        EXPECT_EQ(vector[i], i);
      }
    }

    vector.deinitialize();

    done.notify();
  };

  const auto& dims = pando::getPlaceDims();
  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(dims.node.id), pando::Status::Success);
  for (std::int16_t node = 0; node < dims.node.id; ++node) {
    EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{node}, pando::anyPod, pando::anyCore},
                               f, notification.getHandle(node)),
              pando::Status::Success);
  }
  notification.wait();
}

TEST(InnerVector, Assign) {
  auto f = +[](pando::Notification::HandleType done) {
    constexpr uint64_t size = 1000;

    // create data vector
    pando::Vector<std::uint64_t> dataVector;
    EXPECT_EQ(dataVector.initialize(0), pando::Status::Success);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(dataVector.pushBack(i), pando::Status::Success);
    }
    EXPECT_EQ(dataVector.size(), size);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(dataVector[i], i);
    }

    // create local vector
    galois::InnerVector<std::uint64_t> vector;
    EXPECT_EQ(vector.initialize(0), pando::Status::Success);
    EXPECT_EQ(vector.assign(pando::GlobalPtr<pando::Vector<std::uint64_t>>(&dataVector)),
              pando::Status::Success);
    EXPECT_EQ(vector.size(), size);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(dataVector[i], i);
    }

    // destroy vectors
    vector.deinitialize();
    dataVector.deinitialize();

    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             notification.getHandle()),
            pando::Status::Success);
  notification.wait();
}

TEST(InnerVector, Append) {
  auto f = +[](pando::Notification::HandleType done) {
    constexpr std::uint64_t size = 1000;
    constexpr std::uint64_t numAppends = 4;

    pando::Vector<std::uint64_t> dataVector;
    EXPECT_EQ(dataVector.initialize(0), pando::Status::Success);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(dataVector.pushBack(i), pando::Status::Success);
    }
    EXPECT_EQ(dataVector.size(), size);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(dataVector[i], i);
    }

    // create local vector
    galois::InnerVector<std::uint64_t> lvec;
    EXPECT_EQ(lvec.initialize(0), pando::Status::Success);

    // append vectors to local vector
    for (std::uint64_t i = 0; i < numAppends; i++) {
      EXPECT_EQ(lvec.append(&dataVector), pando::Status::Success);
    }

    // check if local vector has all the data from the other vectors
    EXPECT_EQ(lvec.size(), size * numAppends);
    for (std::uint64_t j = 0; j < numAppends; ++j) {
      for (std::uint64_t k = 0; k < size; ++k) {
        EXPECT_EQ(lvec[j * size + k], k);
      }
    }

    // destroy vectors
    lvec.deinitialize();
    dataVector.deinitialize();

    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             notification.getHandle()),
            pando::Status::Success);
  notification.wait();
}

TEST(InnerVector, AssignRemote) {
  constexpr std::uint64_t SIZE = 1000;
  auto createVector = +[](uint64_t size) -> pando::Vector<std::uint64_t> {
    pando::Vector<std::uint64_t> vec;
    PANDO_CHECK(vec.initialize(size));
    for (std::uint64_t i = 0; i < size; i++) {
      vec[i] = i;
    }
    return vec;
  };
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  pando::LocalStorageGuard gvecGuard(gvec, 1);
  const auto place = pando::Place(pando::NodeIndex(1), pando::anyPod, pando::anyCore);
  *gvec = PANDO_EXPECT_CHECK(pando::executeOnWait(place, createVector, SIZE));
  galois::InnerVector<std::uint64_t> localVec{};
  EXPECT_EQ(localVec.assign(gvec), pando::Status::Success);
  for (std::uint64_t i = 0; i < localVec.size(); i++) {
    EXPECT_EQ(localVec[i], i);
  }
  localVec.deinitialize();
  pando::Vector<std::uint64_t> tvec = *gvec;
  tvec.deinitialize();
}

TEST(InnerVector, RangeLoop) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = 0;
  for (auto val : vec) {
    EXPECT_EQ(val, i);
    i++;
  }
  EXPECT_EQ(size, i);

  vec.deinitialize();
}

TEST(InnerVector, ConstRangeLoop) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = 0;
  for (const auto val : vec) {
    EXPECT_EQ(val, i);
    i++;
  }
  EXPECT_EQ(size, i);

  vec.deinitialize();
}

TEST(InnerVector, Iterator) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = 0;
  for (auto it = vec.begin(); it != vec.end(); it++) {
    EXPECT_EQ(*it, i);
    i++;
  }

  vec.deinitialize();
}

TEST(InnerVector, ConstIterator) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = 0;
  for (auto it = vec.cbegin(); it != vec.cend(); it++) {
    EXPECT_EQ(*it, i);
    i++;
  }

  vec.deinitialize();
}

TEST(InnerVector, ReverseIterator) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = size - 1;
  for (auto it = vec.rbegin(); it != vec.rend(); it++) {
    EXPECT_EQ(*it, i);
    i--;
  }

  vec.deinitialize();
}

TEST(InnerVector, ReverseConstIterator) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(0), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(vec.pushBack(i), pando::Status::Success);
  }
  std::uint64_t i = size - 1;
  for (auto it = vec.crbegin(); it != vec.crend(); it++) {
    EXPECT_EQ(*it, i);
    i--;
  }

  vec.deinitialize();
}

TEST(InnerVector, Equality) {
  constexpr std::uint64_t size = 1000;

  galois::InnerVector<std::uint64_t> vec0;
  galois::InnerVector<std::uint64_t> vec1;
  EXPECT_EQ(vec0.initialize(size), pando::Status::Success);
  EXPECT_EQ(vec1.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  auto it0 = vec0.begin();
  auto it1 = vec1.begin();
  for (; it0 != vec0.end() && it1 != vec1.end(); it0++, it1++, i++) {
    *it0 = i;
    *it1 = i;
  }
  EXPECT_TRUE(vec0 == vec0);
  EXPECT_TRUE(vec1 == vec1);
  EXPECT_TRUE(vec0 == vec1);

  constexpr std::uint64_t newSize = 1025;

  for (std::uint64_t i = size; i < newSize; i++) {
    EXPECT_EQ(vec0.pushBack(i), pando::Status::Success);
    EXPECT_EQ(vec1.pushBack(i), pando::Status::Success);

    EXPECT_TRUE(vec0 == vec1);
  }

  // TODO(muhaawad-amd) this does a shallow copy
  auto func = +[](pando::Notification::HandleType done, galois::InnerVector<std::uint64_t> vec0,
                  galois::InnerVector<std::uint64_t> vec1) {
    galois::InnerVector<std::uint64_t> vec2;
    EXPECT_EQ(vec2.initialize(newSize), pando::Status::Success);
    for (auto it = vec2.begin(); it != vec2.end(); it++) {
      *it = 0;
    }
    EXPECT_TRUE(vec0 == vec1);
    EXPECT_FALSE(vec0 == vec2);
    vec2.deinitialize();

    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func,
                             notification.getHandle(), vec0, vec1),
            pando::Status::Success);
  notification.wait();
  vec0.deinitialize();
  vec1.deinitialize();
}

TEST(InnerVector, Inequality) {
  constexpr std::uint64_t size = 10;

  galois::InnerVector<std::uint64_t> vec0;
  galois::InnerVector<std::uint64_t> vec1;
  EXPECT_EQ(vec0.initialize(size), pando::Status::Success);
  EXPECT_EQ(vec1.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  auto it0 = vec0.begin();
  auto it1 = vec1.begin();
  for (; it0 != vec0.end() && it1 != vec1.end(); it0++, it1++, i++) {
    *it0 = i;
    *it1 = i;
  }
  EXPECT_FALSE(vec0 != vec0);
  EXPECT_FALSE(vec1 != vec1);
  EXPECT_FALSE(vec0 != vec1);

  constexpr std::uint64_t newSize = 17;
  for (std::uint64_t i = size; i < newSize; i++) {
    EXPECT_EQ(vec0.pushBack(i), pando::Status::Success);
    EXPECT_EQ(vec1.pushBack(i + 1), pando::Status::Success);

    EXPECT_TRUE(vec0 != vec1);
  }

  auto func = +[](pando::Notification::HandleType done, galois::InnerVector<std::uint64_t> vec0,
                  galois::InnerVector<std::uint64_t> vec1) {
    galois::InnerVector<std::uint64_t> vec2;
    EXPECT_EQ(vec2.initialize(newSize), pando::Status::Success);
    for (auto it = vec2.begin(); it != vec2.end(); it++) {
      *it = 0;
    }
    EXPECT_TRUE(vec0 != vec1);
    EXPECT_TRUE(vec0 != vec2);

    vec2.deinitialize();
    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func,
                             notification.getHandle(), vec0, vec1),
            pando::Status::Success);
  notification.wait();

  vec0.deinitialize();
  vec1.deinitialize();
}

TEST(InnerVector, LocalityIterator) {
  galois::InnerVector<pando::Array<std::uint64_t>> innVec;
  EXPECT_EQ(innVec.initialize(pando::getNodeDims().id), pando::Status::Success);
  for (std::int64_t nodeIdx = 0; nodeIdx < pando::getNodeDims().id; nodeIdx++) {
    pando::Array<std::uint64_t> arr;
    EXPECT_EQ(
        arr.initialize(1, pando::Place(pando::NodeIndex(nodeIdx), pando::anyPod, pando::anyCore),
                       pando::MemoryType::Main),
        pando::Status::Success);
    innVec[nodeIdx] = arr;
  }
  std::int64_t nodeIdx = 0;
  for (auto it = innVec.begin(); it != innVec.end(); it++) {
    EXPECT_EQ(localityOf(it).node.id, nodeIdx);
    nodeIdx++;
  }
  EXPECT_EQ(nodeIdx, pando::getNodeDims().id);
}
