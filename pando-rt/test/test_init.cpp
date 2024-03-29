// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/locality.hpp"

TEST(Init, MultipleNodes) {
  const auto place = pando::getPlaceDims();
  EXPECT_GT(place.node.id, 1);
}

TEST(Init, CP) {
  const auto place = pando::getCurrentPlace();
  EXPECT_EQ(place.pod, (pando::PodIndex{-1, -1}));
  EXPECT_EQ(place.core, (pando::CoreIndex{-1, -1}));
}

TEST(Init, CPThread) {
  const auto threadIdx = pando::getCurrentThread();
  EXPECT_EQ(threadIdx, (pando::ThreadIndex{-1}));
}
