// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_
#define PANDO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_

#include "utils.hpp"

template <typename GraphType>
void tc_no_chunk(pando::GlobalPtr<GraphType> graph_ptr,
                 galois::DAccumulator<uint64_t> final_tri_count);

template <typename GraphType>
void tc_chunk_edges(pando::GlobalPtr<GraphType> graph_ptr,
                    galois::DAccumulator<uint64_t> final_tri_count);

template <typename GraphType>
void tc_chunk_vertices(pando::GlobalPtr<GraphType> graph_ptr,
                       galois::DAccumulator<uint64_t> final_tri_count);

void HBMainTC(pando::Notification::HandleType hb_done, pando::Array<char> filename,
              int64_t num_vertices, bool load_balanced_graph, TC_CHUNK tc_chunk,
              galois::DAccumulator<uint64_t> final_tri_count);

#endif // PANDO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_ALGOS_HPP_
