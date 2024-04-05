// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.  */

#include <gtest/gtest.h>

#include "pando-rt/containers/vector.hpp"

#include "pando-rt/execution/execute_on.hpp"
#include "pando-rt/sync/notification.hpp"

#include "../common.hpp"

TEST(Vector, Empty) {
  pando::Vector<std::uint64_t> vector;
  EXPECT_EQ(vector.initialize(0), pando::Status::Success);
  EXPECT_TRUE(vector.empty());
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(vector.capacity(), 0);

  vector.deinitialize();
  EXPECT_EQ(vector.capacity(), 0);
  EXPECT_EQ(vector.size(), 0);
}

TEST(Vector, Initialize) {
  const std::uint64_t size = 10;

  pando::Vector<std::uint64_t> vector;
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

TEST(Vector, PushBack) {
  const std::uint64_t size = 10;
  const std::uint64_t newCap = 16;

  pando::Vector<std::uint64_t> vector;
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

TEST(Vector, Clear) {
  const std::uint64_t size = 10;

  pando::Vector<std::uint64_t> vector;
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

TEST(Vector, RemotePushBack) {
  auto f = +[](pando::Notification::HandleType done) {
    auto pushBackF = +[](pando::Notification::HandleType done,
                         pando::GlobalPtr<pando::Vector<std::uint64_t>> vectorPtr) {
      auto vector = static_cast<pando::Vector<std::uint64_t>>(*vectorPtr);
      EXPECT_EQ(vector.initialize(0), pando::Status::Success);
      EXPECT_EQ(vector.reserve(1), pando::Status::Success);
      EXPECT_EQ(vector.pushBack(1u), pando::Status::Success);
      EXPECT_EQ(vector.pushBack(2u), pando::Status::Success);
      EXPECT_EQ(vector.size(), 2);
      *vectorPtr = vector;
      done.notify();
    };

    pando::Vector<std::uint64_t> vector;
    pando::Notification innerNotification;
    EXPECT_EQ(innerNotification.init(), pando::Status::Success);
    EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                               pushBackF, innerNotification.getHandle(),
                               pando::GlobalPtr<pando::Vector<std::uint64_t>>(&vector)),
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

TEST(Vector, StressCreateDestroy) {
  const std::uint64_t requests = 10;
  pando::NotificationArray notifications;
  EXPECT_EQ(notifications.init(requests), pando::Status::Success);
  for (std::uint64_t i = 0; i < requests; i++) {
    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore},
                  +[](pando::Notification::HandleType done) {
                    constexpr auto size = 1;
                    pando::Vector<std::uint64_t> vec;
                    EXPECT_EQ(vec.initialize(size), pando::Status::Success);
                    vec.deinitialize();
                    done.notify();
                  },
                  notifications.getHandle(i)),
              pando::Status::Success);
  }
  notifications.wait();
}

TEST(Vector, StressPushBack) {
  const std::uint64_t size = 8;
  const std::uint64_t finalSz = 1 << 6;

  pando::Vector<std::uint64_t> vector;
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

TEST(Vector, MultiNodePushBack) {
  auto f = +[](pando::Notification::HandleType done) {
    const std::uint64_t size = 8;
    const std::uint64_t finalSz = 1 << 8;

    pando::Vector<std::uint64_t> vector;
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

TEST(Vector, Assign) {
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
    pando::Vector<std::uint64_t> vector;
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

TEST(Vector, Append) {
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
    pando::Vector<std::uint64_t> lvec;
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

TEST(Vector, RangeLoop) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, ConstRangeLoop) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, Iterator) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, ConstIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, ReverseIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, ReverseConstIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec;
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

TEST(Vector, Equality) {
  constexpr std::uint64_t size = 1000;

  pando::Vector<std::uint64_t> vec0;
  pando::Vector<std::uint64_t> vec1;
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
  auto func = +[](pando::Notification::HandleType done, pando::Vector<std::uint64_t> vec0,
                  pando::Vector<std::uint64_t> vec1) {
    pando::Vector<std::uint64_t> vec2;
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

TEST(Vector, Inequality) {
  constexpr std::uint64_t size = 10;

  pando::Vector<std::uint64_t> vec0;
  pando::Vector<std::uint64_t> vec1;
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

  auto func = +[](pando::Notification::HandleType done, pando::Vector<std::uint64_t> vec0,
                  pando::Vector<std::uint64_t> vec1) {
    pando::Vector<std::uint64_t> vec2;
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
