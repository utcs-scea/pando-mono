// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <pando-rt/export.h>

#include <variant>

#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/sync/notification.hpp>

TEST(DISABLED_Issue327, ProofOfExistence) {
  pando::Notification notification;
  EXPECT_EQ(pando::executeOn(
                pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore},
                +[](pando::Notification::HandleType done) {
                  EXPECT_EQ(true, false);
                  done.notify();
                },
                notification.getHandle()),
            pando::Status::Success);
  EXPECT_TRUE(notification.waitFor(std::chrono::seconds(10)));
}
