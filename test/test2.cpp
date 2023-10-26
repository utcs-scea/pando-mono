/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-lib-galois/loops.hpp"
#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"

TEST(Init, CP) {
  auto place = pando::getCurrentPlace();
  EXPECT_EQ(place.core, (pando::CoreIndex{-1, -1}));
}

TEST(Init, GALOIS) {
  auto core = galois::dummy();
  EXPECT_EQ(core, 0);
}
