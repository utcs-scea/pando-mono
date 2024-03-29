// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <tuple>

#include <pando-rt/sync/notification.hpp>

#include "../common.hpp"

TEST(Notification, NotificationWait) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_FALSE(notification.done());
  auto handle = notification.getHandle();
  handle.notify();
  notification.wait();
  EXPECT_TRUE(notification.done());
}

TEST(Notification, NotificationWaitFor) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  auto handle = notification.getHandle();
  handle.notify();
  EXPECT_TRUE(notification.waitFor(std::chrono::milliseconds(1)));
  EXPECT_TRUE(notification.done());
}

TEST(Notification, NotificationWaitForTimeout) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_FALSE(notification.waitFor(std::chrono::milliseconds(1)));
  EXPECT_FALSE(notification.done());
}

TEST(Notification, NotificationExplicitFlag) {
  auto flag = static_cast<pando::GlobalPtr<bool>>(malloc(pando::MemoryType::Main, sizeof(bool)));
  pando::Notification notification;
  EXPECT_EQ(notification.init(flag), pando::Status::Success);
  auto handle = notification.getHandle();
  handle.notify();
  EXPECT_TRUE(notification.waitFor(std::chrono::milliseconds(1)));
  free(flag, sizeof(bool));
}

TEST(Notification, NotificationReset) {
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  auto handle = notification.getHandle();
  handle.notify();
  notification.wait();
  EXPECT_TRUE(notification.done());
  notification.reset();
  EXPECT_FALSE(notification.done());
}

TEST(Notification, NotificationArrayWait) {
  const auto size = 32u;

  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(size), pando::Status::Success);
  EXPECT_FALSE(notification.done());

  for (auto i = 0u; i < size; ++i) {
    auto handle = notification.getHandle(i);
    handle.notify();

    if (i < size - 1) {
      // unless all notify() calls are made, done() should be false
      EXPECT_FALSE(notification.done());
    } else {
      // this is the last notify() call
      EXPECT_TRUE(notification.done());
    }
  }

  notification.wait();
  EXPECT_TRUE(notification.done());
}

TEST(Notification, NotificationArrayWaitFor) {
  const auto size = 32u;

  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(size), pando::Status::Success);

  for (auto i = 0u; i < size; ++i) {
    auto handle = notification.getHandle(i);
    handle.notify();
  }

  EXPECT_TRUE(notification.waitFor(std::chrono::milliseconds(1)));
  EXPECT_TRUE(notification.done());
}

TEST(Notification, NotificationArrayWaitForTimeout) {
  const auto size = 32u;

  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(size), pando::Status::Success);

  // notify all but one
  for (auto i = 0u; i < size - 1; ++i) {
    auto handle = notification.getHandle(i);
    handle.notify();
  }

  EXPECT_FALSE(notification.waitFor(std::chrono::milliseconds(1)));
  EXPECT_FALSE(notification.done());
}

TEST(Notification, NotificationArrayExplicitFlag) {
  const auto size = 32u;

  auto flags =
      static_cast<pando::GlobalPtr<bool>>(malloc(pando::MemoryType::Main, size * sizeof(bool)));
  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(flags, size), pando::Status::Success);
  for (auto i = 0u; i < size; ++i) {
    auto handle = notification.getHandle(i);
    handle.notify();
  }
  EXPECT_TRUE(notification.waitFor(std::chrono::milliseconds(1)));
  free(flags, size * sizeof(bool));
}

TEST(Notification, NotificationArrayReset) {
  const auto size = 32u;

  pando::NotificationArray notification;
  EXPECT_EQ(notification.init(size), pando::Status::Success);

  for (auto i = 0u; i < size; ++i) {
    auto handle = notification.getHandle(i);
    handle.notify();
  }

  notification.wait();
  EXPECT_TRUE(notification.done());
  notification.reset();
  EXPECT_FALSE(notification.done());
}
