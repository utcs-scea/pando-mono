// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>

#include "pando-rt/sync/atomic.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"
#include "pando-rt/memory/allocate_memory.hpp"
#include "pando-rt/memory/global_ptr.hpp"

namespace {

template <typename T, std::int16_t NodeIndex>
struct AtomicsTestData {
  using ValueType = T;
  static constexpr auto nodeIndex = NodeIndex;
};

template <class TestData>
struct AtomicsTest : public ::testing::Test {
  using ValueType = typename TestData::ValueType;
  static constexpr auto nodeIndex = TestData::nodeIndex;

  pando::GlobalPtr<ValueType> ptr{};

  void SetUp() override {
    const pando::Place place{pando::NodeIndex{nodeIndex}, pando::anyPod, pando::anyCore};
    auto expectedPtr = pando::allocateMemory<ValueType>(1, place, pando::MemoryType::Main);
    ASSERT_TRUE(expectedPtr.hasValue());
    ptr = expectedPtr.value();
  }

  void TearDown() override {
    pando::deallocateMemory(ptr, 1);
  }
};

#define GENERATE_TEST_DATA(T, NodeIndex) AtomicsTestData<T, NodeIndex>
// Test different T types and local (NodeIndex = 0) and remote (NodeIndex = 1) operations
#define GENERATE_TEST_TYPES(T) GENERATE_TEST_DATA(T, 0), GENERATE_TEST_DATA(T, 1)

using AtomicsTypes =
    ::testing::Types<GENERATE_TEST_TYPES(std::int32_t), GENERATE_TEST_TYPES(std::uint32_t),
                     GENERATE_TEST_TYPES(std::int64_t), GENERATE_TEST_TYPES(std::uint64_t)>;

#undef GENERATE_TEST_DATA
#undef GENERATE_TEST_TYPES

TYPED_TEST_SUITE(AtomicsTest, AtomicsTypes);

} // namespace

TYPED_TEST(AtomicsTest, ValueBasedLoad) {
  using ValueType = typename TestFixture::ValueType;
  const ValueType gold{32};
  *this->ptr = gold;
  ValueType found = pando::atomicLoad(this->ptr, std::memory_order_relaxed);

  EXPECT_EQ(found, gold);
}

TYPED_TEST(AtomicsTest, ValueBasedStore) {
  using ValueType = typename TestFixture::ValueType;
  const ValueType gold{32};
  pando::atomicStore(this->ptr, gold, std::memory_order_relaxed);

  EXPECT_EQ(*this->ptr, gold);
}

TYPED_TEST(AtomicsTest, ValueBasedCompareExchange) {
  using ValueType = typename TestFixture::ValueType;
  ValueType expected{32};
  ValueType oldValue{expected};
  ValueType desired{64};
  *this->ptr = expected;
  const auto successOrder = std::memory_order_relaxed;
  const auto failureOrder = std::memory_order_relaxed;
  const bool success = pando::atomicCompareExchangeBool(this->ptr, expected, desired, successOrder, failureOrder);

  EXPECT_TRUE(success);
  EXPECT_EQ(expected, oldValue);
  EXPECT_EQ(*this->ptr, desired);
}

TYPED_TEST(AtomicsTest, Increment) {
  using ValueType = typename TestFixture::ValueType;
  using GlobalPtrType = pando::GlobalPtr<ValueType>;

  auto test = [](GlobalPtrType ptr) {
    const ValueType oldExpected{32};
    const ValueType toAdd{5};

    *ptr = oldExpected;
    const auto order = std::memory_order_relaxed;
    pando::atomicIncrement(ptr, toAdd, order);

    EXPECT_EQ(*ptr, oldExpected + toAdd);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  const auto result = pando::executeOnWait(place, +test, this->ptr);
  EXPECT_TRUE(result.hasValue());
}

TYPED_TEST(AtomicsTest, Decrement) {
  using ValueType = typename TestFixture::ValueType;
  using GlobalPtrType = pando::GlobalPtr<ValueType>;

  auto test = [](GlobalPtrType ptr) {
    const ValueType oldExpected{32};
    const ValueType toSubtract{5};

    *ptr = oldExpected;
    const auto order = std::memory_order_relaxed;
    pando::atomicDecrement(ptr, toSubtract, order);
    EXPECT_EQ(*ptr, oldExpected - toSubtract);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  const auto result = pando::executeOnWait(place, +test, this->ptr);
  EXPECT_TRUE(result.hasValue());
}

TYPED_TEST(AtomicsTest, ValueBasedFetchAdd) {
  using ValueType = typename TestFixture::ValueType;
  using GlobalPtrType = pando::GlobalPtr<ValueType>;

  auto test = [](GlobalPtrType ptr) {
    ValueType oldExpected{32};
    ValueType toAdd{5};
    *ptr = oldExpected;
    const auto order = std::memory_order_relaxed;
    const auto oldFound = pando::atomicFetchAdd(ptr, toAdd, order);

    EXPECT_EQ(oldFound, oldExpected);
    EXPECT_EQ(*ptr, oldExpected + toAdd);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  const auto result = pando::executeOnWait(place, +test, this->ptr);
  EXPECT_TRUE(result.hasValue());
}

TYPED_TEST(AtomicsTest, ValueBasedFetchSub) {
  using ValueType = typename TestFixture::ValueType;
  using GlobalPtrType = pando::GlobalPtr<ValueType>;

  auto test = [](GlobalPtrType ptr) {
    ValueType oldExpected{32};
    ValueType toSubtract{5};
    *ptr = oldExpected;
    const auto order = std::memory_order_relaxed;
    const auto oldFound = pando::atomicFetchSub(ptr, toSubtract, order);

    EXPECT_EQ(oldFound, oldExpected);
    EXPECT_EQ(*ptr, oldExpected - toSubtract);
  };

  const auto thisPlace = pando::getCurrentPlace();
  const auto place = pando::Place{thisPlace.node, pando::anyPod, pando::anyCore};
  const auto result = pando::executeOnWait(place, +test, this->ptr);
  EXPECT_TRUE(result.hasValue());
}
