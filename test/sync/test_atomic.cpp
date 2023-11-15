// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/atomic.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory.hpp>
#include <pando-rt/memory/memory_utilities.hpp>
#include <pando-rt/pando-rt.hpp>

namespace {

template <typename T>
pando::GlobalPtr<T> initPrimitiveGlobalPtr() {
  const auto expected =
      pando::allocateMemory<T>(1, pando::getCurrentPlace(), pando::MemoryType::Main);
  EXPECT_EQ(expected.hasValue(), true);
  return expected.value();
}

struct DebugState {
  DebugState() = default;

  pando::Status initialize() {
    pos = initPrimitiveGlobalPtr<double>();
    neg = initPrimitiveGlobalPtr<double>();
    *pos = 0;
    *neg = 0;
    EXPECT_FLOAT_EQ(*pos, 0);
    EXPECT_FLOAT_EQ(*neg, 0);
    return pando::Status::Success;
  }

  pando::GlobalPtr<double> pos;
  pando::GlobalPtr<double> neg;
};

} // namespace

TEST(Atomic, Init) {
  pando::GlobalPtr<double> x;
  x = initPrimitiveGlobalPtr<double>();
  double a = 7.550;
  *x = a;
  EXPECT_FLOAT_EQ(*x, a);
  a += 1.51;
  EXPECT_EQ(pando::atomicFetchAdd(x, 1.51), a);
  a -= 12.0777;
  EXPECT_EQ(pando::atomicFetchSub(x, 12.0777), a);
}

TEST(Atomic, Parallel) {
  uint64_t size = 10000;
  double scale = 1.0;
  pando::Vector<double> doubles;
  EXPECT_EQ(doubles.initialize(size), pando::Status::Success);
  for (uint64_t i = 0; i < size; i++) {
    doubles[i] = scale;
  }

  DebugState state;
  EXPECT_EQ(state.initialize(), pando::Status::Success);

  galois::doAll(
      state, doubles, +[](DebugState state, double update) {
        pando::atomicFetchAdd(state.pos, update);
        pando::atomicFetchSub(state.neg, update);
      });

  EXPECT_FLOAT_EQ(*state.pos, size * scale);
  EXPECT_FLOAT_EQ(*state.neg, size * (-1 * scale));
}
