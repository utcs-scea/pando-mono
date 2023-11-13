// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/utility/string_view.hpp>

TEST(StringView, Constructor) {
  const char* hi = "hihihi";
  const char* bye = "byebyebye";
  const char* hi_substr = "hihi";
  const char* bye_substr = "byebye";
  const char* empty = "";

  galois::StringView hi_view(hi);
  EXPECT_EQ(hi_view.get(), hi);
  EXPECT_EQ(hi_view.size(), 6);
  EXPECT_EQ(hi_view.empty(), false);

  galois::StringView bye_view(bye);
  EXPECT_EQ(bye_view.get(), bye);
  EXPECT_EQ(bye_view.size(), 9);
  EXPECT_EQ(bye_view.empty(), false);

  galois::StringView hi_view_sized(hi, 4);
  galois::StringView hi_view_substr(hi_substr);
  EXPECT_EQ(hi_view_sized, hi_view_substr);
  EXPECT_EQ(hi_view_sized.size(), 4);
  EXPECT_EQ(hi_view_substr.empty(), false);

  galois::StringView bye_view_sized(bye, 6);
  galois::StringView bye_view_substr(bye_substr);
  EXPECT_EQ(bye_view_sized, bye_view_substr);
  EXPECT_EQ(bye_view_sized.size(), 6);
  EXPECT_EQ(bye_view_substr.empty(), false);

  galois::StringView empty_view(empty);
  galois::StringView empty_view_substr(hi, 0);
  EXPECT_EQ(empty_view, empty_view_substr);
  EXPECT_EQ(empty_view.size(), 0);
  EXPECT_EQ(empty_view.empty(), true);
  EXPECT_EQ(empty_view_substr.empty(), true);
}

TEST(StringView, Parse) {
  galois::StringView zero("0");
  galois::StringView one("1");
  galois::StringView twelve("12");
  galois::StringView one_hundred_two("102");
  galois::StringView five_million("5738230");
  galois::StringView eighty_million("85738230");
  galois::StringView two_billion("2035738230");

  EXPECT_EQ(zero.getU64(), 0);
  EXPECT_EQ(one.getU64(), 1);
  EXPECT_EQ(twelve.getU64(), 12);
  EXPECT_EQ(one_hundred_two.getU64(), 102);
  EXPECT_EQ(five_million.getU64(), 5738230);
  EXPECT_EQ(eighty_million.getU64(), 85738230);
  EXPECT_EQ(two_billion.getU64(), 2035738230);
}
