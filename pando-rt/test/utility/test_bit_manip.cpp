// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <pando-rt/utility/bit_manip.hpp>

TEST(BitManip, BitRange) {
  const auto lo = 23;
  const auto hi = 33;
  const pando::BitRange bits{lo, hi};
  EXPECT_EQ(bits.lo, lo);
  EXPECT_EQ(bits.hi, hi);
  EXPECT_EQ(bits.width(), 10);
}

TEST(BitManip, ReadBits) {
  const std::uint64_t bitPattern =
      0b00111011'11000000'00000000'00000000'00000000'00000000'00000000'00000000;
  const std::uint64_t expectedValue = 0b11101111;
  const pando::BitRange bits{54, 62};
  EXPECT_EQ(pando::readBits(bitPattern, bits), expectedValue);
}

TEST(BitManip, CreateMask) {
  const std::uint64_t value = 0b11101111;
  const std::uint64_t expectedBitPattern =
      0b00111011'11000000'00000000'00000000'00000000'00000000'00000000'00000000;
  const pando::BitRange bits{54, 62};
  EXPECT_EQ(pando::createMask(bits, value), expectedBitPattern);
}
