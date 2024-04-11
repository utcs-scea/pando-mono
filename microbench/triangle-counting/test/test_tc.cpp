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
                 TC_CHUNK tc_chunk, GRAPH_TYPE graph_type) {
  galois::DAccumulator<uint64_t> final_tri_count;
  EXPECT_EQ(final_tri_count.initialize(), pando::Status::Success);
  pando::Notification necessary;
  PANDO_CHECK(necessary.init());
  PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                               &HBMainTC, necessary.getHandle(), filename, num_vertices, tc_chunk,
                               graph_type, final_tri_count));
  necessary.wait();
  EXPECT_EQ(final_tri_count.reduce(), expected_tc);
  final_tri_count.deinitialize();
}

class TriangleCountChunking
    : public ::testing::TestWithParam<
          std::tuple<const char*, uint64_t, uint64_t, TC_CHUNK, GRAPH_TYPE>> {};
TEST_P(TriangleCountChunking, BasicDL) {
  const std::string elFile = std::get<0>(GetParam());
  const uint64_t num_vertices = (uint64_t)std::get<1>(GetParam());
  const uint64_t expected_tc = std::get<2>(GetParam());
  TC_CHUNK tc_chunk = std::get<3>(GetParam());
  GRAPH_TYPE graph_type = std::get<4>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  e2e_tc_test(expected_tc, filename, num_vertices, tc_chunk, graph_type);

  filename.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, TriangleCountChunking,
    ::testing::Values(std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::NO_CHUNK, GRAPH_TYPE::DLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_VERTICES, GRAPH_TYPE::DLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_EDGES, GRAPH_TYPE::DLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::NO_CHUNK, GRAPH_TYPE::MDLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_VERTICES, GRAPH_TYPE::MDLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::CHUNK_EDGES, GRAPH_TYPE::MDLCSR),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale5_nV32_nE153.el", 32,
                                      401, TC_CHUNK::NO_CHUNK, GRAPH_TYPE::DACSR)));
