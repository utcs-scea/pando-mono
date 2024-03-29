// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>

#include "pando-rt/utility/function.hpp"

#include "../common.hpp"

TEST(Function, Empty) {
  pando::Function<void()> f;
  EXPECT_EQ(static_cast<bool>(f), false);
}

TEST(Function, Nullptr) {
  pando::Function<void()> f(nullptr);
  EXPECT_EQ(static_cast<bool>(f), false);
}

TEST(Function, FunctionPointer) {
  pando::Function<std::int64_t()> f(&fun);
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(), 42);
}

TEST(Function, FunctionNoexceptPointer) {
  pando::Function<std::int64_t()> f(&funNoexcept);
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(), 42);
}

TEST(Function, FunctionObject) {
  pando::Function<std::int64_t()> f(FunctionObject{});
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(), 42);
}

TEST(Function, LargeFunctionObject) {
  pando::Function<std::int64_t()> f(LargeFunctionObject{});
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(), 42);
}

TEST(Function, MemberFunction) {
  FunctionObject o;
  pando::Function<std::int64_t(const FunctionObject&)> f(&FunctionObject::memFun);
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(o), 43);
}

TEST(Function, Argument) {
  pando::Function<void(std::int64_t)> f(&funI);
  ASSERT_EQ(static_cast<bool>(f), true);
  f(42);
}

TEST(Function, MultipleCalls) {
  pando::Function<std::int64_t()> f(FunctionObject{});
  ASSERT_EQ(static_cast<bool>(f), true);
  EXPECT_EQ(f(), 42);
}

TEST(Function, Copy) {
  pando::Function<std::int64_t()> f(FunctionObject{});
  auto ff = f;
  ASSERT_EQ(static_cast<bool>(f), true);
  ASSERT_EQ(static_cast<bool>(ff), true);
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(ff(), 42);
}

TEST(Function, Move) {
  pando::Function<std::int64_t()> f(FunctionObject{});
  auto ff = std::move(f);
  ASSERT_EQ(static_cast<bool>(f), false);
  ASSERT_EQ(static_cast<bool>(ff), true);
  EXPECT_EQ(ff(), 42);
}

TEST(Function, CopyAssign) {
  pando::Function<std::int64_t()> f(FunctionObject{}), ff;
  ff = f;
  ASSERT_EQ(static_cast<bool>(f), true);
  ASSERT_EQ(static_cast<bool>(ff), true);
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(ff(), 42);
}

TEST(Function, MoveAssign) {
  pando::Function<std::int64_t()> f(FunctionObject{}), ff;
  ff = std::move(f);
  ASSERT_EQ(static_cast<bool>(f), false);
  ASSERT_EQ(static_cast<bool>(ff), true);
  EXPECT_EQ(ff(), 42);
}

TEST(Function, AssignNullptr) {
  pando::Function<std::int64_t()> f(FunctionObject{});
  f = nullptr;
  ASSERT_EQ(static_cast<bool>(f), false);
}
