// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "pando-rt/execution/task.hpp"

#include "../common.hpp"

TEST(Task, CallableFunctionPtr) {
  static bool done = false;

  auto f = +[] {
    EXPECT_FALSE(done);
    done = true;
  };

  pando::Task task(f);
  task();

  EXPECT_TRUE(done);
}

TEST(Task, CallableFunction) {
  static bool done = false;

  auto f = [] {
    EXPECT_FALSE(done);
    done = true;
  };

  pando::Task task(f);
  task();

  EXPECT_TRUE(done);
}

TEST(Task, CallableFunctionObject) {
  static bool done = false;

  struct FunctionObject {
    void operator()() {
      EXPECT_FALSE(done);
      done = true;
    }
  };

  FunctionObject f;

  pando::Task task(f);
  task();

  EXPECT_TRUE(done);
}

TEST(Task, ArgumentBool) {
  const bool t = true;
  auto f = [](bool b) {
    EXPECT_EQ(b, true);
  };

  pando::Task task(f, t);
  task();
}

TEST(Task, ArgumentInt) {
  const int t = 42;
  auto f = [](int i) {
    EXPECT_EQ(i, 42);
  };

  pando::Task task(f, t);
  task();
}

TEST(Task, ArgumentEmptyClass) {
  const EmptyClass t;
  auto f = [](const EmptyClass& e) {
    const EmptyClass other;
    EXPECT_EQ(e, other);
  };

  pando::Task task(f, t);
  task();
}

TEST(Task, ArgumentAggregate) {
  const Aggregate t = {1, 2, true, 4, 5};
  auto f = [](Aggregate a) {
    const Aggregate other = {1, 2, true, 4, 5};
    EXPECT_EQ(a, other);
  };

  pando::Task task(f, t);
  task();
}

TEST(Task, ArgumentVector) {
  const auto t = createVector();

  auto f = [](const std::vector<std::int32_t>& v) {
    const auto other = createVector();
    EXPECT_EQ(v, other);
  };

  pando::Task task(f, t);
  task();
}

TEST(Task, CopyElision) {
  CountingObject t;

  auto f = [](CountingObject v) {
    EXPECT_EQ(v.moves, 2);
    EXPECT_EQ(v.copies, 0);
  };

  pando::Task task(f, std::move(t));
  task();

  EXPECT_EQ(t.moves, 0);
  EXPECT_EQ(t.copies, 0);
}

TEST(Task, Postamble) {
  std::int32_t called = 0;
  auto postamble = [&called]() mutable {
    ++called;
  };

  const auto t = createVector();

  auto f = [](const std::vector<std::int32_t>& v) {
    const auto other = createVector();
    EXPECT_EQ(v, other);
  };

  pando::Task task(pando::Task::withPostamble, postamble, f, t);
  task();

  EXPECT_EQ(called, 1);
}
