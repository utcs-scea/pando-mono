// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-tc-galois/tc_algos.hpp>

uint64_t get_expected_TC(const std::string okFile) {
  std::ifstream f(okFile);

  uint64_t expected_tc = 0;
  f >> expected_tc;
  return expected_tc;
}

void e2e_tc_DFS_test(uint64_t expected_tc, pando::Array<char> filename, uint64_t numVertices,
                     bool load_balanced_graph) {
  galois::DAccumulator<uint64_t> final_tri_count;
  EXPECT_EQ(final_tri_count.initialize(), pando::Status::Success);
  pando::Notification necessary;
  PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                               &HBMainDFS, necessary.getHandle(), filename, numVertices,
                               load_balanced_graph, final_tri_count));
  necessary.wait();
  EXPECT_EQ(final_tri_count.reduce(), expected_tc);
  final_tri_count.deinitialize();
}

class TriangleCountDFS
    : public ::testing::TestWithParam<std::tuple<const char*, const char*, std::uint64_t>> {};
TEST_P(TriangleCountDFS, BasicDL) {
  const std::string elFile = std::get<0>(GetParam());
  const std::string okFile = std::get<1>(GetParam());
  const std::uint64_t numVertices = std::get<2>(GetParam());

  uint64_t expected_tc = get_expected_TC(okFile);

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_DFS_test(expected_tc, filename, numVertices, true);

  filename.deinitialize();
}

TEST_P(TriangleCountDFS, BasicDA) {
  const std::string elFile = std::get<0>(GetParam());
  const std::string okFile = std::get<1>(GetParam());
  const std::uint64_t numVertices = std::get<2>(GetParam());

  uint64_t expected_tc = get_expected_TC(okFile);

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_DFS_test(expected_tc, filename, numVertices, false);

  filename.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, TriangleCountDFS,
    ::testing::Values(
        // std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el",
        // "/pando/ok/rmat_571919_seed1_scale5_nV32_nE153.el-32.ok", 32),
        // std::make_tuple("/pando/graphs/rmat_571919_seed1_scale6_nV64_nE403.el",
        // "/pando/ok/rmat_571919_seed1_scale6_nV64_nE403.el-64.ok", 64),
        // std::make_tuple("/pando/graphs/rrmat_571919_seed1_scale7_nV128_nE942.el",
        // "/pando/ok/rmat_571919_seed1_scale7_nV128_nE942.el-128.ok", 128),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale8_nV256_nE2144.el",
                        "/pando/ok/rmat_571919_seed1_scale8_nV256_nE2144.el-256.ok", 256)));
// std::make_tuple("/pando/graphs/rmat_571919_seed1_scale9_nV512_nE4736.el",
// "/pando/ok/rmat_571919_seed1_scale9_nV512_nE4736.el-512.ok", 512),
// std::make_tuple("/pando/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el",
// "/pando/ok/rmat_571919_seed1_scale10_nV1024_nE10447.el-1024.ok", 1024)
