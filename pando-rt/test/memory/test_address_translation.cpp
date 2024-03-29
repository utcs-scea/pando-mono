// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <pando-rt/memory/address_translation.hpp>

#if defined(PANDO_RT_USE_BACKEND_PREP)

TEST(AddressTranslation, ExtractMemoryTypeUnknown) {
  const pando::GlobalAddress ptr =
      0b11111100'10101010'10101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::Unknown);
}

TEST(AddressTranslation, ExtractMemoryTypeL1SP) {
  const pando::GlobalAddress ptr =
      0b00000000'10101010'10101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::L1SP);
}

TEST(AddressTranslation, ExtractMemoryTypeL2SP) {
  const pando::GlobalAddress ptr =
      0b00000100'10101010'10101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::L2SP);
}

TEST(AddressTranslation, ExtractMemoryTypeMain) {
  const pando::GlobalAddress ptr =
      0b00001000'10101010'10101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::Main);
}

TEST(AddressTranslation, ExtractNodeIndex) {
  const pando::NodeIndex idx{0b11101010101011};
  const pando::GlobalAddress ptr =
      0b00000011'10101010'10110000'00000000'00000000'00000000'00000000'00000000;
  EXPECT_EQ(pando::extractNodeIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractPodIndex) {
  const pando::PodIndex idx{0b101, 0b111};
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000000'01111010'00000000'00000000'00000000;
  EXPECT_EQ(pando::extractPodIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractCoreIndex) {
  const pando::CoreIndex idx{0b101, 0b111};
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000000'00000001'11101000'00000000'00000000;
  EXPECT_EQ(pando::extractCoreIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractGlobalBit) {
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000000'00000000'00000100'00000000'00000000;
  EXPECT_EQ(pando::extractL1SPGlobalBit(ptr), true);
}

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

TEST(AddressTranslation, ExtractMemoryTypeL1SP) {
  const pando::GlobalAddress ptr =
      0b00000000'10101010'00101010'10101010'10101000'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::L1SP);
}

TEST(AddressTranslation, ExtractMemoryTypeL2SP) {
  const pando::GlobalAddress ptr =
      0b00000100'10101010'00101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::L2SP);
}

TEST(AddressTranslation, ExtractMemoryTypeMain) {
  const pando::GlobalAddress ptr =
      0b00001000'10101010'10101010'10101010'10101010'10101010'10101010'10101010;
  EXPECT_EQ(pando::extractMemoryType(ptr), pando::MemoryType::Main);
}

TEST(AddressTranslation, ExtractNodeIndex) {
  const pando::NodeIndex idx{0b11101010101011};
  const pando::GlobalAddress ptr =
      0b00000011'10001010'11110101'01010110'00000000'00000000'00000000'00000000;
  EXPECT_EQ(pando::extractNodeIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractPodIndex) {
  const pando::PodIndex idx{0b101111, 0b0};
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000000'10111110'00000000'00000000'00000000;
  EXPECT_EQ(pando::extractPodIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractCoreIndex) {
  const pando::CoreIndex idx{0b101, 0b111};
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000000'00000001'11111010'00000000'00000000;
  EXPECT_EQ(pando::extractCoreIndex(ptr), idx);
}

TEST(AddressTranslation, ExtractGlobalBit) {
  const pando::GlobalAddress ptr =
      0b00000000'00000000'00000000'00000001'00000000'00000000'00000000'00000000;
  EXPECT_EQ(pando::extractL1SPGlobalBit(ptr), true);
}

#endif
