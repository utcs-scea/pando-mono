// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>

#include "pando-rt/utility/expected.hpp"

TEST(Expected, Value) {
  const std::int32_t value = 42;

  pando::Expected<std::int32_t> e(value);
  EXPECT_EQ(static_cast<bool>(e), true);
  EXPECT_EQ(e.hasValue(), true);
  EXPECT_EQ(e.value(), value);
}

TEST(Expected, Error) {
  const auto value = pando::Status::NotImplemented;

  pando::Expected<std::int32_t> e(value);
  EXPECT_EQ(static_cast<bool>(e), false);
  EXPECT_EQ(e.hasValue(), false);
  EXPECT_EQ(e.error(), value);
}
