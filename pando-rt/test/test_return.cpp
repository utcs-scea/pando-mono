// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#include <gtest/gtest.h>

#include <pando-rt/pando-rt.hpp>

pando::Status testFunc(pando::Status err, bool& retEarly) {
  retEarly = true;
  PANDO_CHECK_RETURN(err);
  retEarly = false;
  return err;
}

TEST(PandoCheckReturn, Simple) {
  bool retEarly;
  EXPECT_EQ(pando::Status::BadAlloc, testFunc(pando::Status::BadAlloc, retEarly));
  EXPECT_TRUE(retEarly);
  EXPECT_EQ(pando::Status::Success, testFunc(pando::Status::Success, retEarly));
  EXPECT_FALSE(retEarly);
}

TEST(PANDO_EXPECT_RETURN, Success) {
  auto success = +[]() -> pando::Status {
    const std::int32_t value = 42;

    std::int32_t v = PANDO_EXPECT_RETURN(pando::Expected<std::int32_t>(value));
    EXPECT_EQ(v, value);
    return pando::Status::Error;
  };
  EXPECT_EQ(pando::Status::Error, success());
}

TEST(PANDO_EXPECT_RETURN, Fail) {
  auto returnFailure = +[]() -> pando::Status {
    const auto value = pando::Status::NotImplemented;

    std::int32_t v = PANDO_EXPECT_RETURN(pando::Expected<std::int32_t>(value));
    EXPECT_TRUE(false) << "Should not have gotten here";
    EXPECT_EQ(v, 100);
    return pando::Status::Error;
  };
  EXPECT_EQ(pando::Status::NotImplemented, returnFailure());
}
