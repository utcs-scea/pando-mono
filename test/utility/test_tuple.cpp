// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/pando-rt.hpp>

TEST(Tuple1, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  auto tpl1 = make_tpl(10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl1, arr, +[](decltype(tpl1) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val] = tpl;
        v = val;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull);
  }
  arr.deinitialize();
}

TEST(Tuple2, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t NUM = 2;
  auto tpl2 = make_tpl(10ull, 10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl2, arr, +[](decltype(tpl2) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val1, val2] = tpl;
        v = val1 + val2;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull * NUM);
  }
  arr.deinitialize();
}

TEST(Tuple3, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t NUM = 3;
  auto tpl3 = make_tpl(10ull, 10ull, 10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl3, arr, +[](decltype(tpl3) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val1, val2, val3] = tpl;
        v = val1 + val2 + val3;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull * NUM);
  }
  arr.deinitialize();
}

TEST(Tuple4, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t NUM = 4;
  auto tpl4 = make_tpl(10ull, 10ull, 10ull, 10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl4, arr, +[](decltype(tpl4) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val1, val2, val3, val4] = tpl;
        v = val1 + val2 + val3 + val4;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull * NUM);
  }
  arr.deinitialize();
}

TEST(Tuple5, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t NUM = 5;
  auto tpl5 = make_tpl(10ull, 10ull, 10ull, 10ull, 10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl5, arr, +[](decltype(tpl5) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val1, val2, val3, val4, val5] = tpl;
        v = val1 + val2 + val3 + val4 + val5;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull * NUM);
  }
  arr.deinitialize();
}

TEST(Tuple6, DoAll) {
  using galois::make_tpl;
  constexpr std::uint64_t size = 10;
  constexpr std::uint64_t NUM = 6;
  auto tpl6 = make_tpl(10ull, 10ull, 10ull, 10ull, 10ull, 10ull);
  pando::Array<std::uint64_t> arr{};
  EXPECT_EQ(pando::Status::Success, arr.initialize(size));
  auto err = galois::doAll(
      tpl6, arr, +[](decltype(tpl6) tpl, pando::GlobalRef<std::uint64_t> v) {
        auto [val1, val2, val3, val4, val5, val6] = tpl;
        v = val1 + val2 + val3 + val4 + val5 + val6;
      });
  EXPECT_EQ(pando::Status::Success, err);
  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], 10ull * NUM);
  }
  arr.deinitialize();
}

TEST(Tuple1, get) {
  using galois::make_tpl;
  auto tpl1 = make_tpl(10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl1));
}

TEST(Tuple2, get) {
  using galois::make_tpl;
  auto tpl2 = make_tpl(10ull, 10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl2));
  EXPECT_EQ(10ull, std::get<1>(tpl2));
}
TEST(Tuple3, get) {
  using galois::make_tpl;
  auto tpl3 = make_tpl(10ull, 10ull, 10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl3));
  EXPECT_EQ(10ull, std::get<1>(tpl3));
  EXPECT_EQ(10ull, std::get<2>(tpl3));
}

TEST(Tuple4, get) {
  using galois::make_tpl;
  auto tpl4 = make_tpl(10ull, 10ull, 10ull, 10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl4));
  EXPECT_EQ(10ull, std::get<1>(tpl4));
  EXPECT_EQ(10ull, std::get<2>(tpl4));
  EXPECT_EQ(10ull, std::get<3>(tpl4));
}

TEST(Tuple5, get) {
  using galois::make_tpl;
  auto tpl5 = make_tpl(10ull, 10ull, 10ull, 10ull, 10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl5));
  EXPECT_EQ(10ull, std::get<1>(tpl5));
  EXPECT_EQ(10ull, std::get<2>(tpl5));
  EXPECT_EQ(10ull, std::get<3>(tpl5));
  EXPECT_EQ(10ull, std::get<4>(tpl5));
}

TEST(Tuple6, get) {
  using galois::make_tpl;
  auto tpl6 = make_tpl(10ull, 10ull, 10ull, 10ull, 10ull, 10ull);
  EXPECT_EQ(10ull, std::get<0>(tpl6));
  EXPECT_EQ(10ull, std::get<1>(tpl6));
  EXPECT_EQ(10ull, std::get<2>(tpl6));
  EXPECT_EQ(10ull, std::get<0>(tpl6));
  EXPECT_EQ(10ull, std::get<1>(tpl6));
  EXPECT_EQ(10ull, std::get<2>(tpl6));
}
