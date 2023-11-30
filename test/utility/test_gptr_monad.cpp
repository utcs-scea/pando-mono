// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(Fmap, GVectorInitialize) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  fmap(*gvec, initialize, SIZE);
  pando::Vector<std::uint64_t> vec = *gvec;
  EXPECT_EQ(vec.size(), SIZE);
  pando::deallocateMemory(gvec, 1);
}

TEST(Fmap, GVectorPushBack) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  fmap(*gvec, initialize, 0);

  for (std::uint64_t i = 0; i < SIZE; i++) {
    fmap(*gvec, pushBack, i);
  }

  pando::Vector<std::uint64_t> vec = *gvec;
  EXPECT_EQ(vec.size(), SIZE);
  std::uint64_t i = 0;
  for (std::uint64_t v : vec) {
    EXPECT_EQ(v, i);
    i++;
  }
  EXPECT_EQ(SIZE, i);
  pando::deallocateMemory(gvec, 1);
}
