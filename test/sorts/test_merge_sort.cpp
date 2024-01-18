// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <pando-rt/export.h>
#include <stdlib.h>

#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/sorts/merge_sort.hpp>

TEST(Sorts, BasicDesSort) {
  pando::Vector<std::uint64_t> arr;
  auto const size = 10;
  PANDO_CHECK(arr.initialize(size));

  for (int i = 0; i < size; i++) {
    arr[i] = i;
  }

  galois::merge_sort<std::uint64_t>(arr, [](std::uint64_t a, std::uint64_t b) {
    return a < b;
  });
  for (int i = 0; i < (size - 1); i++) {
    EXPECT_EQ(arr[i] >= arr[i + 1], true);
  }
}

TEST(Sorts, BasicAscSort) {
  pando::Vector<std::uint64_t> arr;
  auto const size = 10;
  PANDO_CHECK(arr.initialize(size));

  for (int i = 0; i < size; i++) {
    arr[i] = size - i;
  }

  galois::merge_sort<std::uint64_t>(arr, [](std::uint64_t a, std::uint64_t b) {
    return a > b;
  });
  for (int i = 0; i < (size - 1); i++) {
    EXPECT_EQ(arr[i] <= arr[i + 1], true);
  }
}

TEST(Sorts, RandAscSort) {
  pando::Vector<std::uint64_t> arr;
  auto const size = 10;
  PANDO_CHECK(arr.initialize(size));

  unsigned int seed;
  for (int i = 0; i < size; i++) {
    arr[i] = rand_r(&seed) % size * 10 + 1;
  }

  galois::merge_sort<std::uint64_t>(arr, [](std::uint64_t a, std::uint64_t b) {
    return a > b;
  });
  for (int i = 0; i < (size - 1); i++) {
    EXPECT_EQ(arr[i] <= arr[i + 1], true);
  }
}

TEST(Sorts, RandDesSort) {
  pando::Vector<std::uint64_t> arr;
  auto const size = 10;
  PANDO_CHECK(arr.initialize(size));

  unsigned int seed;
  for (int i = 0; i < size; i++) {
    arr[i] = rand_r(&seed) % size * 10 + 1;
  }

  galois::merge_sort<std::uint64_t>(arr, [](std::uint64_t a, std::uint64_t b) {
    return a < b;
  });
  for (int i = 0; i < (size - 1); i++) {
    EXPECT_EQ(arr[i] >= arr[i + 1], true);
  }
}

TEST(Sorts, RandSmallAscSort) {
  pando::Vector<std::uint64_t> arr;
  auto const size = 10;
  PANDO_CHECK(arr.initialize(size));

  unsigned int seed;
  for (int i = 0; i < size; i++) {
    arr[i] = rand_r(&seed) % size / 2 + 1;
  }

  galois::merge_sort<std::uint64_t>(arr, [](std::uint64_t a, std::uint64_t b) {
    return a > b;
  });
  for (int i = 0; i < (size - 1); i++) {
    EXPECT_EQ(arr[i] <= arr[i + 1], true);
  }
}
