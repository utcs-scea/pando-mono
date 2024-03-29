// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <pando-rt/index.hpp>

using pando::anyPlace, pando::anyNode, pando::anyPod, pando::anyCore;
using pando::isSubsetOf;
using pando::Place, pando::NodeIndex, pando::PodIndex, pando::CoreIndex;

TEST(NodeIndex, SubsetChecks) {
  EXPECT_TRUE(isSubsetOf(NodeIndex{10}, anyNode));
  EXPECT_TRUE(isSubsetOf(anyNode, anyNode));
  EXPECT_TRUE(isSubsetOf(NodeIndex{12}, NodeIndex{12}));
  EXPECT_FALSE(isSubsetOf(anyNode, NodeIndex{15}));
}

TEST(PodIndex, SubsetChecks) {
  EXPECT_TRUE(isSubsetOf(PodIndex{10, 20}, anyPod));
  EXPECT_TRUE(isSubsetOf(anyPod, anyPod));
  EXPECT_TRUE(isSubsetOf(PodIndex{15, 4}, PodIndex{15, 4}));
  EXPECT_FALSE(isSubsetOf(anyPod, PodIndex{20, 10}));
}

TEST(CoreIndex, SubsetChecks) {
  EXPECT_TRUE(isSubsetOf(CoreIndex{10, 20}, anyCore));
  EXPECT_TRUE(isSubsetOf(anyCore, anyCore));
  EXPECT_TRUE(isSubsetOf(CoreIndex{15, 4}, CoreIndex{15, 4}));
  EXPECT_FALSE(isSubsetOf(anyCore, CoreIndex{20, 10}));
}
TEST(Place, SubsetChecks) {
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}}, anyPlace));
  EXPECT_TRUE(isSubsetOf(anyPlace, anyPlace));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{NodeIndex{1}, PodIndex{5, 4}, anyCore}));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{NodeIndex{1}, anyPod, CoreIndex{3, 26}}));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{anyNode, PodIndex{5, 4}, CoreIndex{3, 26}}));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{anyNode, anyPod, CoreIndex{3, 26}}));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{anyNode, PodIndex{5, 4}, anyCore}));
  EXPECT_TRUE(isSubsetOf(Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}},
                         Place{NodeIndex{1}, anyPod, anyCore}));
  EXPECT_FALSE(isSubsetOf(anyPlace, Place{NodeIndex{1}, PodIndex{5, 4}, CoreIndex{3, 26}}));
  EXPECT_FALSE(isSubsetOf(Place{anyNode, PodIndex{5, 4}, CoreIndex{3, 26}},
                          Place{NodeIndex{1}, anyPod, anyCore}));
}
