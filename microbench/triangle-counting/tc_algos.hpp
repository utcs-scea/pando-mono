// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_TC_ALGOS_HPP_
#define PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_TC_ALGOS_HPP_
#include "utils.hpp"
void HBMainDFS(pando::Notification::HandleType hb_done, pando::Array<char> filename,
               int64_t num_vertices, bool load_balanced_graph,
               galois::DAccumulator<uint64_t> final_tri_count);
#endif // PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_TC_ALGOS_HPP_
