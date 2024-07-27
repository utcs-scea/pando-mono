// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <pando-bfs-galois/sssp.hpp>
#include <pando-lib-galois/containers/inner_vector.hpp>

void bfs::updateData(std::uint64_t val, pando::GlobalRef<std::uint64_t> ref) {
  std::uint64_t temp = pando::atomicLoad(&ref, std::memory_order_relaxed);
  do {
    if (val >= temp) {
      break;
    }
  } while (!pando::atomicCompareExchange(&ref, temp, val, std::memory_order_relaxed,
                                         std::memory_order_relaxed));
}

template <>
void bfs::BFSPerHostLoop_DLCSR<bfs::DLCSR>(
    BFSState<bfs::DLCSR> state,
    pando::GlobalRef<pando::Vector<typename bfs::DLCSR::VertexTopologyID>> vecRef) {
  using G = DLCSR;
  pando::Vector<typename G::VertexTopologyID> vec = vecRef;
  galois::InnerVector<typename G::VertexTopologyID> innVec(std::move(vec));
  PANDO_CHECK(galois::doAll(state, innVec, &BFSOuterLoop_DLCSR<G>));
}

bfs::CountEdges<bfs::COUNT_EDGE> bfs::countEdges;
