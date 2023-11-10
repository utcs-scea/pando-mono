
// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(CountedIterator, Array) {
  constexpr std::uint64_t SIZE = 10;
  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(SIZE), pando::Status::Success);

  auto curr = galois::CountedIterator(0, arr.begin());
  const auto end = galois::CountedIterator(SIZE, arr.end());

  std::uint64_t i = 0;
  for (; curr != end; curr++, i++) {
    std::pair<std::uint64_t, pando::GlobalRef<std::uint64_t>> pair = *curr;
    EXPECT_EQ(pair.first, i);
    pair.second = i;
  }

  EXPECT_EQ(i, SIZE);

  i = 0;
  for (std::uint64_t val : arr) {
    EXPECT_EQ(val, i);
    i++;
  }
  EXPECT_EQ(i, SIZE);
}
