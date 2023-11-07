// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/export.h"
#include <pando-rt/pando-rt.hpp>

int pandoMain(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = 0;
  if (pando::getCurrentPlace().node.id == 0) {
    result = RUN_ALL_TESTS();
  }
  pando::waitAll();
  return result;
}
