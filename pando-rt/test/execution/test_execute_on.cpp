// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/execution/execute_on.hpp"

#include "pando-rt/sync/notification.hpp"

namespace {

void functionWithNotification(pando::Notification::HandleType handle) {
  handle.notify();
}

} // namespace

TEST(ExecuteOn, ThisNode) {
  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(place, &functionWithNotification, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST(ExecuteOn, ThisNodeAnyPod) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(pando::anyPod, &functionWithNotification, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST(ExecuteOn, ThisNodeAnyCore) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(pando::anyCore, &functionWithNotification, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST(ExecuteOn, OtherNode) {
  const auto nodeIdx = pando::getCurrentNode();
  const auto nodeDims = pando::getNodeDims();
  const auto otherNodeId = (nodeIdx.id + 1) % nodeDims.id;
  const pando::Place place{pando::NodeIndex(otherNodeId), pando::PodIndex{0, 0},
                           pando::CoreIndex{0, 0}};

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(place, &functionWithNotification, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST(ExecuteOn, Stress) {
  auto f = +[](pando::Notification::HandleType done) {
    const std::uint64_t times = 16;
    const auto& dims = pando::getPlaceDims();
    for (std::uint64_t i = 0; i < times; i++) {
      for (std::int16_t j = 0; j < dims.node.id; j++) {
        pando::Notification innerNotification;
        EXPECT_EQ(innerNotification.init(), pando::Status::Success);
        EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{j}, pando::anyPod, pando::anyCore},
                                   &functionWithNotification, innerNotification.getHandle()),
                  pando::Status::Success);
        innerNotification.wait();
      }
    }

    done.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                             notification.getHandle()),
            pando::Status::Success);
  notification.wait();
}
