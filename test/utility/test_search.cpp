// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/search.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(BinarySearch, Simple) {
  constexpr std::uint64_t SIZE = 16;
  pando::Vector<std::uint64_t> vec;
  EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
  for (std::uint64_t i = 0; i < SIZE; i++) {
    vec[i] = 2 * i;
  }
  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_TRUE(galois::binary_search(vec.begin(), vec.end(), 2 * i));
    EXPECT_FALSE(galois::binary_search(vec.begin(), vec.end(), 2 * i + 1));
  }
  vec.deinitialize();

  EXPECT_EQ(vec.initialize(SIZE + 1), pando::Status::Success);
  for (std::uint64_t i = 0; i < SIZE + 1; i++) {
    vec[i] = 2 * i + 1;
  }
  for (std::uint64_t i = 0; i < SIZE + 1; i++) {
    EXPECT_FALSE(galois::binary_search(vec.begin(), vec.end(), 2 * i));
    EXPECT_TRUE(galois::binary_search(vec.begin(), vec.end(), 2 * i + 1));
  }
}
