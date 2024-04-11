// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/const_range.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(ConstRange, u64) {
  constexpr std::uint64_t SIZE = 10;
  constexpr std::uint64_t RANDOM_VALUE = 9801;
  galois::ConstRange<std::uint64_t> range(SIZE, RANDOM_VALUE);
  std::uint64_t i = 0;
  for (auto thing : range) {
    EXPECT_EQ(thing, RANDOM_VALUE);
    i++;
  }
  EXPECT_EQ(i, SIZE);
}
