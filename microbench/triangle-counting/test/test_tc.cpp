// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <tc_algos.hpp>

uint64_t get_expected_TC(const std::string okFile) {
  std::ifstream f(okFile);

  uint64_t expected_tc = 0;
  f >> expected_tc;
  return expected_tc;
}

void e2e_tc_test(uint64_t expected_tc, pando::Array<char> filename, uint64_t numVertices,
                 bool load_balanced_graph) {
  galois::DAccumulator<uint64_t> final_tri_count;
  EXPECT_EQ(final_tri_count.initialize(), pando::Status::Success);
  pando::Notification necessary;
  PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                               &HBMainTC, necessary.getHandle(), filename, numVertices,
                               load_balanced_graph, final_tri_count));
  necessary.wait();
  EXPECT_EQ(final_tri_count.reduce(), expected_tc);
  final_tri_count.deinitialize();
}

class TriangleCount
    : public ::testing::TestWithParam<std::tuple<const char*, const char*, std::uint64_t>> {};
TEST_P(TriangleCount, BasicDL) {
  const std::string elFile = std::get<0>(GetParam());
  const std::string okFile = std::get<1>(GetParam());
  const std::uint64_t numVertices = std::get<2>(GetParam());

  uint64_t expected_tc = get_expected_TC(okFile);

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_test(expected_tc, filename, numVertices, true);

  filename.deinitialize();
}

TEST_P(TriangleCount, BasicDA) {
  const std::string elFile = std::get<0>(GetParam());
  const std::string okFile = std::get<1>(GetParam());
  const std::uint64_t numVertices = std::get<2>(GetParam());

  uint64_t expected_tc = get_expected_TC(okFile);

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_test(expected_tc, filename, numVertices, false);

  filename.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, TriangleCount,
    ::testing::Values(std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el")));
