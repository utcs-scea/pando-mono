// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_PANGOLIN_HPP_
#define PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_PANGOLIN_HPP_

#include <utility>

#include "utils.hpp"

using VertexList = pando::Vector<Graph::VertexTopologyID>;

struct Embedding {
  VertexList elements;
  Embedding() {
    PANDO_CHECK(elements.initialize(0));
  }
  explicit Embedding(uint64_t n) {
    PANDO_CHECK(elements.initialize(n));
  }
  ~Embedding() {
    elements.deinitialize();
  }
};

struct EmbeddingList {
  uint64_t last_level;
  uint64_t max_level;
  pando::Vector<VertexList> idx_lists;
  pando::Vector<VertexList> vid_lists;

  galois::DistArray<uint64_t> calculate_pfx_offsets(pando::GlobalPtr<Graph> graph_ptr) {
    auto num_vertices = graph_ptr->size();
    galois::DistArray<uint64_t> num_init_embeddings;
    galois::DistArray<uint64_t> start_offsets;
    PANDO_CHECK(num_init_embeddings.initialize(num_vertices));
    PANDO_CHECK(start_offsets.initialize(num_vertices));

    if (num_vertices > 0) {
      // Get numEdges per vertex
      num_init_embeddings[0] = 0;
      for (Graph::VertexTopologyID vid : graph_ptr->vertices()) {
        uint64_t i = graph_ptr->getVertexIndex(vid);
        if (i < num_vertices - 1)
          num_init_embeddings[i + 1] = graph_ptr->getNumEdges(vid);
      }

      using SRC = galois::DistArray<uint64_t>;
      using DST = galois::DistArray<uint64_t>;
      using SRC_Val = uint64_t;
      using DST_Val = uint64_t;

      galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, galois::internal::transmute<uint64_t>,
                        galois::internal::scan_op<SRC_Val, DST_Val>,
                        galois::internal::combiner<DST_Val>, galois::DistArray>
          prefixSum(num_init_embeddings, start_offsets);
      PANDO_CHECK(prefixSum.initialize());
      prefixSum.computePrefixSum(num_vertices);
      prefixSum.deinitialize();
    }
    num_init_embeddings.deinitialize();
    return start_offsets;
  }

  pando::Status initialize(pando::GlobalPtr<Graph> graph_ptr, uint64_t max_size) {
    max_level = max_size;
    last_level = 1;
    uint64_t num_emb = graph_ptr->sizeEdges();

    // Calculate pfx offset where should write info
    galois::DistArray<uint64_t> start_offsets = calculate_pfx_offsets(graph_ptr);

    // Allocate + Write L1 Stuff
    PANDO_CHECK(idx_lists.initialize(max_level));
    PANDO_CHECK(vid_lists.initialize(max_level));

    VertexList idx_list_level1 = idx_lists[1];
    VertexList vid_list_level1 = vid_lists[1];
    PANDO_CHECK(idx_list_level1.initialize(num_emb));
    PANDO_CHECK(vid_list_level1.initialize(num_emb));

    for (uint64_t i = 0; i < start_offsets.size(); i++) {
      uint64_t offset = start_offsets[i];
      Graph::VertexTopologyID vid_src = graph_ptr->getTopologyIDFromIndex(i);

      // TODO(pingle): PARALLELIZE/KERNELIZE Write edges in level 1
      for (typename Graph::EdgeHandle eh : graph_ptr->edges(vid_src)) {
        Graph::VertexTopologyID vid_dest = graph_ptr->getEdgeDst(eh);
        idx_list_level1[offset] = vid_src;
        vid_list_level1[offset] = vid_dest;
        offset++;
      }
    }
    idx_lists[1] = std::move(idx_list_level1);
    vid_lists[1] = std::move(vid_list_level1);
    start_offsets.deinitialize();
    return pando::Status::Success;
  }

  void deinitialize() {
    for (uint64_t i = 0; i < vid_lists.size(); i++) {
      VertexList idx_list_level_i = idx_lists[i];
      VertexList vid_list_level_i = vid_lists[i];
      idx_list_level_i.deinitialize();
      vid_list_level_i.deinitialize();
      idx_lists[i] = std::move(idx_list_level_i);
      vid_lists[i] = std::move(vid_list_level_i);
    }
    idx_lists.deinitialize();
    vid_lists.deinitialize();
  }

  size_t size() const {
    VertexList vid_list_last = vid_lists[last_level];
    return vid_list_last.size();
  }
};

void HBMainPangolin(pando::Notification::HandleType hb_done, pando::Array<char> filename,
                    int64_t num_vertices);

#endif // PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_PANGOLIN_HPP_
