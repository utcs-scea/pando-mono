// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_
#define PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_

#include "tc/utils.hpp"

void HBMainDFS(pando::Notification::HandleType hb_done, pando::Array<char> filename,
               int64_t num_vertices, bool load_balanced_graph, RT_TC_ALGO rt_algo,
               galois::DAccumulator<uint64_t> final_tri_count);

#endif // PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_
