// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef TRIANGLE_COUNTING_INCLUDE_TC_MDLCSR_HPP_
#define TRIANGLE_COUNTING_INCLUDE_TC_MDLCSR_HPP_

#include <utils.hpp>
void updateMasters(typename GraphMDL::VertexData mirrorData,
                   pando::GlobalRef<typename GraphMDL::VertexData> masterData);

void tc_mdlcsr_noChunk(pando::GlobalPtr<GraphMDL> graph_ptr,
                       galois::DAccumulator<uint64_t> final_tri_count);

#endif // TRIANGLE_COUNTING_INCLUDE_TC_MDLCSR_HPP_
