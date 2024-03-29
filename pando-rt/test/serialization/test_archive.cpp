// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "cereal/types/vector.hpp"

#include <pando-rt/serialization/archive.hpp>

#include "../common.hpp"

TEST(Archive, Bool) {
  const bool t = true;

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(t));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  bool u = {};
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, Int) {
  const std::int32_t t = 42;

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(t));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  std::int32_t u = {};
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, EmptyClass) {
  const EmptyClass t;

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, 0u);

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  EmptyClass u;
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, Enum) {
  const Enum t = Enum::Value1;

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(std::underlying_type_t<Enum>));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  Enum u = Enum::Value0;
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, Aggregate) {
  const Aggregate t = {1, 2, true, 4, 5};

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(t));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  Aggregate u{};
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, TriviallyCopyable) {
  const TriviallyCopyable t(42);

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(t));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  TriviallyCopyable u;
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}

TEST(Archive, Vector) {
  const auto t = [] {
    std::vector<std::int32_t> t(100);
    std::iota(t.begin(), t.end(), 2);
    return t;
  }();

  pando::SizeArchive sizeArchive;
  sizeArchive(t);

  const auto byteCount = sizeArchive.byteCount();
  EXPECT_EQ(byteCount, sizeof(t.size()) + t.size() * sizeof(t[0]));

  std::vector<std::byte> buffer(byteCount);
  pando::OutputArchive outputArchive(buffer.data());
  outputArchive(t);

  std::vector<std::int32_t> u;
  pando::InputArchive inputArchive(buffer.data());
  inputArchive(u);

  EXPECT_EQ(t, u);
}
