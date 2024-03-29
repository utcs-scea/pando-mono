// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-rt/execution/execute_on_wait.hpp"

TEST(ExecuteOnWait, ThisNode) {
  const std::int32_t value = 2;

  auto f = +[](pando::Place place, std::int32_t value) -> std::int32_t {
    EXPECT_EQ(place, pando::getCurrentPlace());
    return value;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  auto returnValue = pando::executeOnWait(place, f, place, value);
  EXPECT_TRUE(returnValue.hasValue());
  EXPECT_EQ(returnValue.value(), value);
}

TEST(ExecuteOnWait, ThisNodeVoid) {
  auto f = +[](pando::Place place) -> void {
    EXPECT_EQ(place, pando::getCurrentPlace());
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  auto returnValue = pando::executeOnWait(place, f, place);
  EXPECT_TRUE(returnValue.hasValue());
}

TEST(ExecuteOnWait, ThisNodeNestedCalls) {
  const std::int32_t value = 2;

  auto f = +[](pando::Place place, std::int32_t value) -> std::int32_t {
    EXPECT_EQ(place, pando::getCurrentPlace());

    auto innerF = +[](pando::Place place, std::int32_t value) -> std::int32_t {
      EXPECT_EQ(place, pando::getCurrentPlace());
      return value;
    };

    auto innerReturnValue = pando::executeOnWait(place, innerF, place, value + 1);
    EXPECT_TRUE(innerReturnValue.hasValue());
    EXPECT_EQ(innerReturnValue.value(), value + 1);

    return value;
  };

  const auto thisPlace = pando::getCurrentPlace();
  const pando::Place place{thisPlace.node, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}};

  auto returnValue = pando::executeOnWait(place, f, place, value);
  EXPECT_TRUE(returnValue.hasValue());
  EXPECT_EQ(returnValue.value(), value);
}

TEST(ExecuteOnWait, Stress) {
  auto f = +[] {
    const std::uint64_t times = 16;
    const auto& dims = pando::getPlaceDims();
    for (std::uint64_t i = 0; i < times; i++) {
      for (std::int16_t j = 0; j < dims.node.id; j++) {
        auto innerF = +[] {
          return true;
        };

        auto innerReturnValue = pando::executeOnWait(
            pando::Place{pando::NodeIndex{j}, pando::anyPod, pando::anyCore}, innerF);
        EXPECT_TRUE(innerReturnValue.hasValue());
        EXPECT_TRUE(innerReturnValue.value());
      }
    }
  };

  auto returnValue =
      pando::executeOnWait(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f);
  EXPECT_TRUE(returnValue.hasValue());
}
