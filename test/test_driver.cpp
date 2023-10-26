/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/export.h"
#include <pando-rt/pando-rt.hpp>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto status = pando::initialize();
  if (status != pando::Status::Success) {
    return static_cast<int>(status);
  }
  int result = 0;
  if (pando::getCurrentPlace().node.id == 0) {
    result = RUN_ALL_TESTS();
  }
  pando::waitAll();
  pando::finalize();
  return result;
}
