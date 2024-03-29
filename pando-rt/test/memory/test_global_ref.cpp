// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>

#include "pando-rt/memory/global_ptr.hpp"

#include "../common.hpp"

namespace {

template <typename T>
struct GlobalRefTest : public ::testing::Test {
  pando::GlobalPtr<T> ptr{};

  void SetUp() override {
    ptr = static_cast<pando::GlobalPtr<T>>(malloc(pando::MemoryType::Main, sizeof(T)));
    ASSERT_NE(ptr, nullptr);
  }

  void TearDown() override {
    free(ptr, sizeof(T));
  }
};

using GlobalRefTestTypes =
    ::testing::Types<std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t,
                     std::uint32_t, std::int64_t, std::uint64_t>;
TYPED_TEST_SUITE(GlobalRefTest, GlobalRefTestTypes);

} // namespace

TYPED_TEST(GlobalRefTest, AsignmentOperators) {
  {
    // assign the value of another reference

    const TypeParam expected{2};
    const TypeParam notExpected{3};

    auto anotherPtr = static_cast<pando::GlobalPtr<TypeParam>>(
        malloc(pando::MemoryType::Main, sizeof(TypeParam)));

    EXPECT_EQ(&(*(this->ptr)), this->ptr);
    EXPECT_EQ(&(*(anotherPtr)), anotherPtr);
    EXPECT_NE(this->ptr, anotherPtr);

    *(this->ptr) = notExpected;
    *anotherPtr = expected;
    *(this->ptr) = *anotherPtr;

    EXPECT_EQ(*(this->ptr), expected);
    EXPECT_EQ(&(*(this->ptr)), this->ptr);
    EXPECT_EQ(&(*(anotherPtr)), anotherPtr);
    EXPECT_NE(this->ptr, anotherPtr);

    free(anotherPtr, sizeof(TypeParam));
  }

  {
    // assign a value
    const TypeParam expected{2};
    *(this->ptr) = expected;
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) += value, expected += value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) -= value, expected -= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) *= value, expected *= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) /= value, expected /= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) %= value, expected %= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) &= value, expected &= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) |= value, expected |= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) ^= value, expected ^= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) <<= value, expected <<= value);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    TypeParam expected{2};
    const TypeParam value{3};
    *(this->ptr) = expected;

    EXPECT_EQ(*(this->ptr) >>= value, expected >>= value);
    EXPECT_EQ(*(this->ptr), expected);
  }
}

TYPED_TEST(GlobalRefTest, AsignmentOperatorWithConversion) {
  using ValueType = std::uint64_t;
  pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(pando::MemoryType::Main, sizeof(ValueType)));

  const std::uint32_t expected{32};
  *ptr = expected;
  EXPECT_EQ(*ptr, expected);

  free(ptr, sizeof(ValueType));
}

TYPED_TEST(GlobalRefTest, IncrementDecrementOperators) {
  // Compare results of the pre-/post-increment/decrement
  // operators over GlobalPtr<T> and T.
  TypeParam t{};
  *(this->ptr) = t;
  EXPECT_EQ(*(this->ptr), t);

  EXPECT_EQ(++(*(this->ptr)), ++t);
  EXPECT_EQ(*(this->ptr), t);

  EXPECT_EQ((*(this->ptr))++, t++);
  EXPECT_EQ(*(this->ptr), t);

  EXPECT_EQ(--(*(this->ptr)), --t);
  EXPECT_EQ(*(this->ptr), t);

  EXPECT_EQ((*(this->ptr))--, t--);
  EXPECT_EQ(*(this->ptr), t);
}

TYPED_TEST(GlobalRefTest, UnaryArithmeticOperators) {
  const TypeParam expected{2};
  *(this->ptr) = expected;

  {
    EXPECT_EQ(+(*(this->ptr)), +expected);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    EXPECT_EQ(-(*(this->ptr)), -expected);
    EXPECT_EQ(*(this->ptr), expected);
  }

  {
    EXPECT_EQ(~(*(this->ptr)), ~expected);
    EXPECT_EQ(*(this->ptr), expected);
  }
}

TYPED_TEST(GlobalRefTest, BinaryArithmeticOperators) {
  {
    const TypeParam value1{2};
    const TypeParam value2{7};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) + value2, value1 + value2);
    EXPECT_EQ(value2 + *(this->ptr), value2 + value1);
    EXPECT_EQ(*(this->ptr) + *(this->ptr), value1 + value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{7};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) - value2, value1 - value2);
    EXPECT_EQ(value2 - *(this->ptr), value2 - value1);
    EXPECT_EQ(*(this->ptr) - *(this->ptr), value1 - value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{7};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) * value2, value1 * value2);
    EXPECT_EQ(value2 * *(this->ptr), value2 * value1);
    EXPECT_EQ(*(this->ptr) * *(this->ptr), value1 * value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{31};
    const TypeParam value2{5};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) / value2, value1 / value2);
    EXPECT_EQ(value2 / *(this->ptr), value2 / value1);
    EXPECT_EQ(*(this->ptr) / *(this->ptr), value1 / value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{31};
    const TypeParam value2{5};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) % value2, value1 % value2);
    EXPECT_EQ(value2 % *(this->ptr), value2 % value1);
    EXPECT_EQ(*(this->ptr) % *(this->ptr), value1 % value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{4};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) & value2, value1 & value2);
    EXPECT_EQ(value2 & *(this->ptr), value2 & value1);
    EXPECT_EQ(*(this->ptr) & *(this->ptr), value1 & value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{4};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) | value2, value1 | value2);
    EXPECT_EQ(value2 | *(this->ptr), value2 | value1);
    EXPECT_EQ(*(this->ptr) | *(this->ptr), value1 | value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{4};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) ^ value2, value1 ^ value2);
    EXPECT_EQ(value2 ^ *(this->ptr), value2 ^ value1);
    EXPECT_EQ(*(this->ptr) ^ *(this->ptr), value1 ^ value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{4};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) >> value2, value1 >> value2);
    EXPECT_EQ(value2 >> *(this->ptr), value2 >> value1);
    EXPECT_EQ(*(this->ptr) >> *(this->ptr), value1 >> value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    const TypeParam value1{2};
    const TypeParam value2{4};
    *(this->ptr) = value1;

    EXPECT_EQ(*(this->ptr) << value2, value1 << value2);
    EXPECT_EQ(value2 << *(this->ptr), value2 << value1);
    EXPECT_EQ(*(this->ptr) << *(this->ptr), value1 << value1);

    EXPECT_EQ(*(this->ptr), value1);
  }
}

TYPED_TEST(GlobalRefTest, LogicalOperators) {
  const TypeParam value1{2};
  const TypeParam value2{4};
  *(this->ptr) = value1;

  {
    EXPECT_EQ(!(*(this->ptr)), !value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) && value2, value1 && value2);
    EXPECT_EQ(value2 && *(this->ptr), value2 && value1);
    EXPECT_EQ(*(this->ptr) && *(this->ptr), value1 && value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) || value2, value1 || value2);
    EXPECT_EQ(value2 || *(this->ptr), value2 || value1);
    EXPECT_EQ(*(this->ptr) || *(this->ptr), value1 || value1);

    EXPECT_EQ(*(this->ptr), value1);
  }
}

TYPED_TEST(GlobalRefTest, ComparisonOperators) {
  const TypeParam value1{2};
  const TypeParam value2{7};
  *(this->ptr) = value1;

  {
    EXPECT_EQ(*(this->ptr) == value2, value1 == value2);
    EXPECT_EQ(value2 == *(this->ptr), value2 == value1);
    EXPECT_EQ(*(this->ptr) == *(this->ptr), value1 == value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) != value2, value1 != value2);
    EXPECT_EQ(value2 != *(this->ptr), value2 != value1);
    EXPECT_EQ(*(this->ptr) != *(this->ptr), value1 != value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) < value2, value1 < value2);
    EXPECT_EQ(value2 < *(this->ptr), value2 < value1);
    EXPECT_EQ(*(this->ptr) < *(this->ptr), value1 < value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) > value2, value1 > value2);
    EXPECT_EQ(value2 > *(this->ptr), value2 > value1);
    EXPECT_EQ(*(this->ptr) > *(this->ptr), value1 > value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) <= value2, value1 <= value2);
    EXPECT_EQ(value2 <= *(this->ptr), value2 <= value1);
    EXPECT_EQ(*(this->ptr) <= *(this->ptr), value1 <= value1);

    EXPECT_EQ(*(this->ptr), value1);
  }

  {
    EXPECT_EQ(*(this->ptr) >= value2, value1 >= value2);
    EXPECT_EQ(value2 >= *(this->ptr), value2 >= value1);
    EXPECT_EQ(*(this->ptr) >= *(this->ptr), value1 >= value1);

    EXPECT_EQ(*(this->ptr), value1);
  }
}

TYPED_TEST(GlobalRefTest, Swap) {
  pando::GlobalPtr<TypeParam> x =
      static_cast<pando::GlobalPtr<TypeParam>>(malloc(pando::MemoryType::Main, sizeof(TypeParam)));
  pando::GlobalPtr<TypeParam> y =
      static_cast<pando::GlobalPtr<TypeParam>>(malloc(pando::MemoryType::Main, sizeof(TypeParam)));

  const TypeParam initialX{32};
  const TypeParam initialY{64};

  const auto initialXPtr{x};
  const auto initialYPtr{y};

  *x = initialX;
  *y = initialY;

  pando::swap(*x, *y);

  // values were swapped
  EXPECT_EQ(*x, initialY);
  EXPECT_EQ(*y, initialX);

  // pointers didn't change
  EXPECT_EQ(x, initialXPtr);
  EXPECT_EQ(y, initialYPtr);

  pando::iter_swap(x, y);

  EXPECT_EQ(*x, initialX);
  EXPECT_EQ(*y, initialY);

  // pointers didn't change
  EXPECT_EQ(x, initialXPtr);
  EXPECT_EQ(y, initialYPtr);

  free(x, sizeof(TypeParam));
  free(y, sizeof(TypeParam));
}
