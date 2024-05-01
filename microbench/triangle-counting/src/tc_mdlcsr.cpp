// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <tc_mdlcsr.hpp>

void updateMasters(typename GraphMDL::VertexData mirrorData,
                   pando::GlobalRef<typename GraphMDL::VertexData> masterData) {
  galois::doAll(
      &masterData, mirrorData.queries,
      +[](pando::GlobalPtr<typename GraphMDL::VertexData> masterDataPtr,
          pando::Vector<uint64_t> thread_vector) {
        typename GraphMDL::VertexData masterData = *masterDataPtr;
        for (uint64_t query : thread_vector) {
          PANDO_CHECK(fmap(masterData, add_query, query));
        }
      });
}

void tc_mdlcsr_noChunk(pando::GlobalPtr<GraphMDL> graph_ptr,
                       galois::DAccumulator<uint64_t> final_tri_count) {
  // Init states
  GraphMDL graph = *graph_ptr;

  galois::HostIndexedMap<uint64_t> per_host_wk_remaining{};
  PANDO_CHECK(per_host_wk_remaining.initialize());
  galois::doAll(
      per_host_wk_remaining, +[](pando::GlobalRef<std::uint64_t> wk_remaining_host_i) {
        wk_remaining_host_i = 0;
      });

  auto state = galois::make_tpl(graph_ptr, final_tri_count);

  // Build wedges
  PANDO_MEM_STAT_NEW_KERNEL("BuildWedgesStart");
  galois::doAll(
      graph_ptr, per_host_wk_remaining,
      +[](pando::GlobalPtr<GraphMDL> graph_ptr, pando::GlobalRef<std::uint64_t>) {
        GraphMDL graph = *graph_ptr;

        galois::doAll(
            graph_ptr, graph.getLocalMasterRange(),
            +[](pando::GlobalPtr<GraphMDL> graph_ptr,
                typename GraphMDL::VertexTopologyID local_master_v0) {
              GraphMDL graph = *graph_ptr;
              auto master_tokenID = graph.getTokenID(local_master_v0);
              auto global_topo_id = graph.getGlobalTopologyID(master_tokenID);

              // Build wedges: O(n^2)
              auto edge_range = graph.edges(global_topo_id);
              for (auto edge_it = edge_range.begin(); edge_it != edge_range.end(); edge_it++) {
                typename GraphMDL::EdgeHandle eh = *edge_it;
                typename GraphMDL::VertexTopologyID dst_mirror = graph.getEdgeDst(eh);

                pando::GlobalRef<MirroredVT> dst_mirror_data = graph.getData(dst_mirror);

                // Only set bitset if work exists!
                uint64_t work_sz = edge_range.end() - (edge_it + 1);
                if (work_sz > 0)
                  graph.setBitSet(dst_mirror);
                for (auto edge_inner_it = edge_it + 1; edge_inner_it != edge_range.end();
                     edge_inner_it++) {
                  typename GraphMDL::EdgeHandle other_eh = *edge_inner_it;
                  typename GraphMDL::VertexTopologyID dst_dst_mirror = graph.getEdgeDst(other_eh);
                  uint64_t queried_token_id = graph.getTokenID(dst_dst_mirror);

                  PANDO_CHECK(fmap(dst_mirror_data, add_query, queried_token_id));
                }
              }
            });
      });

  // SYNC
  PANDO_MEM_STAT_NEW_KERNEL("SyncStart");
  graph.sync(updateMasters);

  // Lookup requests
  PANDO_MEM_STAT_NEW_KERNEL("LookupRequestsStart");
  galois::doAll(
      state, per_host_wk_remaining, +[](decltype(state) state, pando::GlobalRef<std::uint64_t>) {
        auto [graph_ptr, _] = state;
        GraphMDL graph = *graph_ptr;

        galois::doAll(
            state, graph.getLocalMasterRange(),
            +[](decltype(state) state, typename GraphMDL::VertexTopologyID local_master_v0) {
              auto [graph_ptr, final_tri_count] = state;
              GraphMDL graph = *graph_ptr;
              auto master_tokenID = graph.getTokenID(local_master_v0);
              auto global_topo_id = graph.getGlobalTopologyID(master_tokenID);

              MirroredVT master_data = graph.getData(local_master_v0);

              auto inner_state = galois::make_tpl(graph_ptr, final_tri_count, global_topo_id);
              galois::doAll(
                  inner_state, master_data.queries,
                  +[](decltype(inner_state) inner_state, pando::Vector<uint64_t> thread_vector) {
                    auto [graph_ptr, final_tri_count, global_topo_id] = inner_state;
                    for (auto query_tokenID : thread_vector) {
                      is_connected_binarySearch<GraphMDL>(graph_ptr, query_tokenID, global_topo_id,
                                                          final_tri_count);
                    }
                  });
            });
      });

  // De-Initialize queries
  PANDO_MEM_STAT_NEW_KERNEL("DeInitStart");
  galois::doAll(
      graph_ptr, graph.vertices(),
      +[](pando::GlobalPtr<GraphMDL> graph_ptr, typename GraphMDL::VertexTopologyID v0) {
        GraphMDL graph = *graph_ptr;

        fmapVoid(graph.getData(v0), deinitialize);
      });

  per_host_wk_remaining.deinitialize();
}
