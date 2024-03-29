// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/containers/array.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"

#include "../common.hpp"

TEST(Array, Empty) {
  pando::Array<std::uint64_t> array;
  EXPECT_EQ(array.initialize(0), pando::Status::Success);
  EXPECT_EQ(array.size(), 0);
  EXPECT_EQ(array.data(), nullptr);
  array.deinitialize();
}

TEST(Array, Initialize) {
  const std::uint64_t size = 10;

  pando::Array<std::uint64_t> array;
  EXPECT_EQ(array.initialize(size), pando::Status::Success);
  EXPECT_EQ(array.size(), size);
  EXPECT_NE(array.data(), nullptr);

  for (std::uint64_t i = 0; i < size; i++) {
    array[i] = i;
  }

  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  array.deinitialize();
}

TEST(Array, Swap) {
  const std::uint64_t size0 = 10;
  const std::uint64_t size1 = 15;

  pando::Array<std::uint64_t> array0;
  EXPECT_EQ(array0.initialize(size0), pando::Status::Success);

  for (std::uint64_t i = 0; i < size0; i++) {
    array0[i] = i;
  }

  for (std::uint64_t i = 0; i < size0; i++) {
    EXPECT_EQ(array0[i], i);
  }

  pando::Array<std::uint64_t> array1;
  EXPECT_EQ(array1.initialize(size1), pando::Status::Success);

  for (std::uint64_t i = 0; i < size1; i++) {
    array1[i] = i + size0;
  }

  for (std::uint64_t i = 0; i < size1; i++) {
    EXPECT_EQ(array1[i], i + size0);
  }

  std::swap(array0, array1);

  for (std::uint64_t i = 0; i < size1; i++) {
    EXPECT_EQ(array0[i], i + size0);
  }

  for (std::uint64_t i = 0; i < size0; i++) {
    EXPECT_EQ(array1[i], i);
  }

  array0.deinitialize();
  array1.deinitialize();
}

TEST(Array, DataAccess) {
  const std::uint64_t size = 1000;

  // create array
  pando::Array<std::uint64_t> array;
  EXPECT_EQ(array.initialize(size), pando::Status::Success);
  for (std::uint64_t i = 0; i < size; i++) {
    array[i] = i;
  }
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(array[i], i);
  }

  auto checker = +[](pando::Span<std::uint64_t> array) {
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(array[i], i);
    }

    return true;
  };

  auto result =
      pando::executeOnWait(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                           checker, pando::Span<std::uint64_t>(array.data(), size));
  EXPECT_TRUE(result.hasValue());
  array.deinitialize();
}

TEST(Array, RangeLoop) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  for (auto val : arr) {
    val = i;
    i++;
  }

  i = 0;
  for (auto val : arr) {
    EXPECT_EQ(val, i);
    i++;
  }
  EXPECT_EQ(size, i);

  arr.deinitialize();
}

TEST(Array, ConstRangeLoop) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  for (auto val : arr) {
    val = i;
    i++;
  }

  i = 0;
  for (const auto val : arr) {
    EXPECT_EQ(val, i);
    i++;
  }
  EXPECT_EQ(size, i);

  arr.deinitialize();
}

TEST(Array, Iterator) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);
  std::uint64_t i = 0;
  for (auto it = arr.begin(); it != arr.end(); it++, i++) {
    *it = i;
  }

  i = 0;
  for (auto it = arr.begin(); it != arr.end(); it++, i++) {
    EXPECT_EQ(*it, i);
  }

  arr.deinitialize();
}

TEST(Array, ConstIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);
  std::uint64_t i = 0;
  for (auto it = arr.begin(); it != arr.end(); it++, i++) {
    *it = i;
  }

  i = 0;
  for (auto it = arr.cbegin(); it != arr.cend(); it++, i++) {
    EXPECT_EQ(*it, i);
  }

  arr.deinitialize();
}

TEST(Array, ReverseIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);
  std::uint64_t i = 0;
  for (auto it = arr.begin(); it != arr.end(); it++, i++) {
    *it = i;
  }

  i = arr.size() - 1;
  for (auto it = arr.rbegin(); it != arr.rend(); it++, i--) {
    EXPECT_EQ(*it, i);
  }

  arr.deinitialize();
}

TEST(Array, ConstReverseIterator) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr;
  EXPECT_EQ(arr.initialize(size), pando::Status::Success);
  std::uint64_t i = 0;
  for (auto it = arr.begin(); it != arr.end(); it++, i++) {
    *it = i;
  }

  i = arr.size() - 1;
  for (auto it = arr.crbegin(); it != arr.crend(); it++, i--) {
    EXPECT_EQ(*it, i);
  }

  arr.deinitialize();
}

TEST(Array, Equality) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr0;
  pando::Array<std::uint64_t> arr1;
  EXPECT_EQ(arr0.initialize(size), pando::Status::Success);
  EXPECT_EQ(arr1.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  auto it0 = arr0.begin();
  auto it1 = arr1.begin();
  for (; it0 != arr0.end() && it1 != arr1.end(); it0++, it1++, i++) {
    *it0 = i;
    *it1 = i;
  }
  EXPECT_TRUE(arr0 == arr0);
  EXPECT_TRUE(arr1 == arr1);
  EXPECT_TRUE(arr0 == arr1);

  // TODO(ypapadop-amd) arr0 and arr1 are shallow copies and should be replaced with a different
  // abstraction (e.g., GlobalRef with appropriate functions implemented on Span<std::uint64_t)
  auto func = +[](pando::Array<std::uint64_t> arr0, pando::Array<std::uint64_t> arr1) {
    pando::Array<std::uint64_t> arr2;
    EXPECT_EQ(arr2.initialize(size), pando::Status::Success);
    for (auto it = arr2.begin(); it != arr2.end(); it++) {
      *it = 0;
    }
    EXPECT_TRUE(arr0 == arr1);
    EXPECT_FALSE(arr0 == arr2);
    arr2.deinitialize();

    return true;
  };

  auto result = pando::executeOnWait(
      pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func, arr0, arr1);
  EXPECT_TRUE(result.hasValue());

  arr0.deinitialize();
  arr1.deinitialize();
}

TEST(Array, Inequality) {
  constexpr std::uint64_t size = 10;

  pando::Array<std::uint64_t> arr0;
  pando::Array<std::uint64_t> arr1;
  EXPECT_EQ(arr0.initialize(size), pando::Status::Success);
  EXPECT_EQ(arr1.initialize(size), pando::Status::Success);

  std::uint64_t i = 0;
  auto it0 = arr0.begin();
  auto it1 = arr1.begin();
  for (; it0 != arr0.end() && it1 != arr1.end(); it0++, it1++, i++) {
    *it0 = i;
    *it1 = i;
  }
  EXPECT_FALSE(arr0 != arr0);
  EXPECT_FALSE(arr1 != arr1);
  EXPECT_FALSE(arr0 != arr1);

  // TODO(ypapadop-amd) arr0 and arr1 are shallow copies and should be replaced with a different
  // abstraction (e.g., GlobalRef with appropriate functions implemented on Span<std::uint64_t)
  auto func = +[](pando::Array<std::uint64_t> arr0, pando::Array<std::uint64_t> arr1) {
    pando::Array<std::uint64_t> arr2;
    EXPECT_EQ(arr2.initialize(size), pando::Status::Success);
    for (auto it = arr2.begin(); it != arr2.end(); it++) {
      *it = 0;
    }
    EXPECT_FALSE(arr0 != arr1);
    EXPECT_TRUE(arr0 != arr2);

    arr2.deinitialize();

    return true;
  };

  auto result = pando::executeOnWait(
      pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, func, arr0, arr1);
  EXPECT_TRUE(result.hasValue());

  arr0.deinitialize();
  arr1.deinitialize();
}

TEST(Array, Equivalence) {
  constexpr std::uint64_t size = 1000;

  pando::Array<std::uint64_t> arr0;
  pando::Array<std::uint64_t> arr1;
  EXPECT_EQ(arr0.initialize(size), pando::Status::Success);
  EXPECT_EQ(arr1.initialize(size), pando::Status::Success);
  EXPECT_TRUE(isSame(arr0, arr0));
  EXPECT_TRUE(isSame(arr1, arr1));
  EXPECT_FALSE(isSame(arr0, arr1));

  arr0.deinitialize();
  arr1.deinitialize();
}
