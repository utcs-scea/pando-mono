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

void e2e_tc_test(uint64_t expected_tc, pando::Array<char> filename, uint64_t num_vertices,
                 bool load_balanced_graph, TC_CHUNK tc_chunk) {
  galois::DAccumulator<uint64_t> final_tri_count;
  EXPECT_EQ(final_tri_count.initialize(), pando::Status::Success);
  HBMainTC(filename, num_vertices, load_balanced_graph, tc_chunk, final_tri_count);
  EXPECT_EQ(final_tri_count.reduce(), expected_tc);
  final_tri_count.deinitialize();
}

class TriangleCountChunking
    : public ::testing::TestWithParam<std::tuple<const char*, uint64_t, uint64_t, TC_CHUNK>> {};
TEST_P(TriangleCountChunking, BasicDL) {
  const std::string elFile = std::get<0>(GetParam());
  const uint64_t num_vertices = (uint64_t)std::get<1>(GetParam());
  const uint64_t expected_tc = std::get<2>(GetParam());
  TC_CHUNK tc_chunk = std::get<3>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_test(expected_tc, filename, num_vertices, true, tc_chunk);

  filename.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, TriangleCountChunking,
    ::testing::Values(std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::NO_CHUNK),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_VERTICES),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_EDGES)));

// Chunking not avail for DACSR
class TriangleCountDACSR
    : public ::testing::TestWithParam<std::tuple<const char*, uint64_t, uint64_t>> {};
TEST_P(TriangleCountDACSR, BasicDA) {
  const std::string elFile = std::get<0>(GetParam());
  const uint64_t num_vertices = std::get<1>(GetParam());
  const uint64_t expected_tc = std::get<2>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_test(expected_tc, filename, num_vertices, false, TC_CHUNK::NO_CHUNK);

  filename.deinitialize();
}
INSTANTIATE_TEST_SUITE_P(SmallFiles, TriangleCountDACSR,
                         ::testing::Values(std::make_tuple(
                             "/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32, 401)));
