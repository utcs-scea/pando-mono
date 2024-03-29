// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/pando-rt.hpp"

int pandoMain(int argc, char** argv) {
  // Googletest modifies argc/argv in place. Duplicate argv and then pass it along.
  std::vector<char*> args(argv, argv + argc + 1);

  ::testing::InitGoogleTest(&argc, args.data());

  int result = 0;
  if (pando::getCurrentPlace().node.id == 0) {
    result = RUN_ALL_TESTS();
  }
  pando::waitAll();
  return result;
}
