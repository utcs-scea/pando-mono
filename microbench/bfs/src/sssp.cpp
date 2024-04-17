// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <pando-bfs-galois/sssp.hpp>

void bfs::updateData(std::uint64_t val, pando::GlobalRef<std::uint64_t> ref) {
  std::uint64_t temp = pando::atomicLoad(&ref, std::memory_order_relaxed);
  do {
    if (val >= temp) {
      break;
    }
  } while (!pando::atomicCompareExchange(&ref, pando::GlobalPtr<std::uint64_t>(&temp),
                                         pando::GlobalPtr<std::uint64_t>(&val),
                                         std::memory_order_relaxed, std::memory_order_relaxed));
}

bfs::CountEdges<galois::COUNT_EDGE> galois::countEdges;
