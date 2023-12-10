// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/utility/pair.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(Pair, Swap) {
  using VPair = galois::Pair<std::uint64_t, pando::Vector<std::uint64_t>>;
  pando::Vector<std::uint64_t> vA;
  PANDO_CHECK(vA.initialize(10));
  pando::Vector<std::uint64_t> vB;
  PANDO_CHECK(vB.initialize(11));
  VPair a{0, vA};
  VPair b{1, vB};
  std::swap(a, b);
  EXPECT_EQ(0, b.first);
  EXPECT_EQ(1, a.first);
  EXPECT_TRUE(a.second == vB);
  EXPECT_TRUE(b.second == vA);
  vA.deinitialize();
  vB.deinitialize();
}

TEST(Pair, VectorSort) {
  using VPair = galois::Pair<std::uint64_t, pando::Vector<std::uint64_t>>;
  const std::uint64_t size = 5;
  pando::Vector<VPair> vector;
  PANDO_CHECK(vector.initialize(0));
  PANDO_CHECK(vector.reserve(size * size));

  for (std::uint64_t i = 0; i < size; i++) {
    for (std::uint64_t j = 0; j < size; j++) {
      pando::Vector<std::uint64_t> v;
      std::uint64_t revI = size - i - 1;
      std::uint64_t revj = size - j - 1;
      PANDO_CHECK(v.initialize(revj));
      PANDO_CHECK(vector.pushBack(VPair{revI, v}));
    }
  }

  EXPECT_EQ(vector.size(), size * size);
  for (std::uint64_t i = 0; i < size; i++) {
    for (std::uint64_t j = 0; j < size; j++) {
      VPair p = vector[size * i + j];
      EXPECT_EQ(size - i - 1, p.first);
    }
  }

  std::sort(vector.begin(), vector.end());

  for (std::uint64_t i = 0; i < size; i++) {
    for (std::uint64_t j = 0; j < size; j++) {
      VPair p = vector[size * i + j];
      EXPECT_EQ(p.first, i);
      p.second.deinitialize();
    }
  }
}
