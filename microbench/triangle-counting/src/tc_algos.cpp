// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <tc_algos.hpp>

// #####################################################################
//                            TC UTILS
// #####################################################################
/**
 * @brief Runs the innermost loop of TC.
 * Given a vertex, constructs an embedding tree from that vertex over subset of v0's edges
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <typename Graph>
void edge_tc_counting(pando::GlobalPtr<Graph> graph_ptr, typename Graph::VertexTopologyID v0,
                      typename Graph::EdgeRange edge_range,
                      galois::DAccumulator<uint64_t> final_tri_count) {
  galois::WaitGroup wg;
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();
  auto inner_state = galois::make_tpl(graph_ptr, v0, wgh, final_tri_count);
  galois::doAll(
      inner_state, edge_range,
      +[](decltype(inner_state) inner_state, typename Graph::EdgeHandle eh) {
        auto [graph_ptr, v0, wgh, final_tri_count] = inner_state;

        Graph g = *graph_ptr;
        wgh.addOne();
        typename Graph::VertexTopologyID v1 = fmap(g, getEdgeDst, eh);
        bool v0_higher_degree = fmap(g, getNumEdges, v0) >= fmap(g, getNumEdges, v1);
        pando::Place locality =
            v0_higher_degree ? fmap(g, getLocalityVertex, v0) : fmap(g, getLocalityVertex, v1);
        PANDO_CHECK(pando::executeOn(locality, &intersect_dag_merge<Graph>, wgh, graph_ptr, v0, v1,
                                     final_tri_count));
      });
  PANDO_CHECK(wg.wait());
}

// #####################################################################
//                        TC IMPLEMENTATIONS
// #####################################################################
/**
 * @brief Runs the Triangle Counting Algorithm
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <typename Graph>
void tc_no_chunk(pando::GlobalPtr<Graph> graph_ptr,
                 galois::DAccumulator<uint64_t> final_tri_count) {
  Graph graph = *graph_ptr;
  auto state = galois::make_tpl(graph_ptr, final_tri_count);
  galois::doAll(
      state, graph.vertices(), +[](decltype(state) state, typename Graph::VertexTopologyID v0) {
        auto [graph_ptr, final_tri_count] = state;
        Graph graph = *graph_ptr;

        // Degree Filtering Optimization
        uint64_t v0_degree = graph.getNumEdges(v0);
        if (v0_degree < (TC_EMBEDDING_SZ - 1))
          return;

        edge_tc_counting<Graph>(graph_ptr, v0, graph.edges(v0), final_tri_count);
      });
}

/**
 * @brief Runs Chunked Edges Triangle Counting Algorithm on DistLocalCSRs (GraphDL)
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <typename Graph>
void tc_chunk_edges(pando::GlobalPtr<Graph> graph_ptr,
                    galois::DAccumulator<uint64_t> final_tri_count) {
  Graph graph = *graph_ptr;
  uint64_t query_sz = 1;
  uint64_t iters = 0;

  galois::DAccumulator<uint64_t> work_remaining{};
  PANDO_CHECK(work_remaining.initialize());

  // Assumption: ELVertex iterator_offset = 0
  do {
    work_remaining.reset();
    auto state = galois::make_tpl(graph_ptr, work_remaining, query_sz, final_tri_count);
    galois::doAll(
        state, graph.vertices(), +[](decltype(state) state, typename Graph::VertexTopologyID v0) {
          auto [graph_ptr, work_remaining, query_sz, final_tri_count] = state;
          Graph g = *graph_ptr;

          // DF Optimization
          uint64_t v0_numEdges = fmap(g, getNumEdges, v0);
          if (v0_numEdges < (TC_EMBEDDING_SZ - 1))
            return;

          // Each vertex stores a bookmark where they left off
          // We modify DistLocalCSR (GraphDL) to allow us to capture a subset of the edgeRange
          uint64_t current_offset = v0->iterator_offset;
          auto curr_edge_range = fmap(g, edges, v0, current_offset, query_sz);
          auto local_work_remaining = curr_edge_range.size();

          edge_tc_counting<Graph>(graph_ptr, v0, curr_edge_range, final_tri_count);

          // Move bookmark forward and flag if this vertex is not yet done
          v0->iterator_offset += local_work_remaining;
          if (v0->iterator_offset < v0_numEdges)
            work_remaining.increment();
        });
#if DEBUG
    uint64_t current_count = final_tri_count.reduce();
    std::cout << "After Iter " << iters << " found " << current_count << " triangles.\n";
    final_tri_count.reset();
    final_tri_count.add(current_count);
#endif
    iters++;
    query_sz <<= 1;
  } while (work_remaining.reduce());
  work_remaining.deinitialize();
}

/**
 * @brief Runs Chunked Vertices Triangle Counting Algorithm on DistLocalCSRs (GraphDL)
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <typename Graph>
void tc_chunk_vertices(pando::GlobalPtr<Graph> graph_ptr,
                       galois::DAccumulator<uint64_t> final_tri_count) {
  Graph graph = *graph_ptr;
  uint64_t num_hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  uint64_t query_sz = 1;
  uint64_t iters = 0;

  galois::DAccumulator<uint64_t> work_remaining{};
  PANDO_CHECK(work_remaining.initialize());

  // Initialize vertexlist offsets to 0
  galois::HostIndexedMap<uint64_t> per_host_iterator_offsets{};
  PANDO_CHECK(per_host_iterator_offsets.initialize());
  galois::doAll(
      per_host_iterator_offsets, +[](pando::GlobalRef<std::uint64_t> host_vertex_iter_offset_ref) {
        host_vertex_iter_offset_ref = 0;
      });

  do {
    work_remaining.reset();
    auto state = galois::make_tpl(graph_ptr, query_sz, work_remaining, final_tri_count);
    galois::doAll(
        state, per_host_iterator_offsets,
        +[](decltype(state) state, pando::GlobalRef<std::uint64_t> host_vertex_iter_offset_ref) {
          auto [graph_ptr, query_sz, work_remaining, final_tri_count] = state;
          Graph graph = *graph_ptr;
          auto lcsr = graph.getLocalCSR();
          uint64_t host_vertex_iter_offset = host_vertex_iter_offset_ref;

          auto inner_state = galois::make_tpl(graph_ptr, final_tri_count);
          galois::doAll(
              inner_state, fmap(lcsr, vertices, host_vertex_iter_offset, query_sz),
              +[](decltype(inner_state) inner_state, typename Graph::VertexTopologyID v0) {
                auto [graph_ptr, final_tri_count] = inner_state;
                Graph graph = *graph_ptr;

                // Degree Filtering Optimization
                uint64_t v0_degree = graph.getNumEdges(v0);
                if (v0_degree < (TC_EMBEDDING_SZ - 1))
                  return;

                edge_tc_counting<Graph>(graph_ptr, v0, graph.edges(v0), final_tri_count);
              });

          // Move iter offset
          uint64_t lcsr_num_vertices = fmap(lcsr, size);
          host_vertex_iter_offset += query_sz;
          if (host_vertex_iter_offset < lcsr_num_vertices)
            work_remaining.increment();
          host_vertex_iter_offset_ref = host_vertex_iter_offset;
        });
#if DEBUG
    uint64_t current_count = final_tri_count.reduce();
    std::cout << "After Iter " << iters << " found " << current_count << " triangles.\n";
    final_tri_count.reset();
    final_tri_count.add(current_count);
#endif
    iters++;
    query_sz <<= 1;
  } while (work_remaining.reduce());

  work_remaining.deinitialize();
  per_host_iterator_offsets.deinitialize();
}

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
  galois::doAll(
      graph_ptr, per_host_wk_remaining,
      +[](pando::GlobalPtr<GraphMDL> graph_ptr,
          pando::GlobalRef<std::uint64_t> wk_remaining_host_i) {
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
  graph.sync(updateMasters);

  // Lookup requests
  galois::doAll(
      state, per_host_wk_remaining,
      +[](decltype(state) state, pando::GlobalRef<std::uint64_t> wk_remaining_host_i) {
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
  galois::doAll(
      graph_ptr, graph.vertices(),
      +[](pando::GlobalPtr<GraphMDL> graph_ptr, typename GraphMDL::VertexTopologyID v0) {
        GraphMDL graph = *graph_ptr;

        fmapVoid(graph.getData(v0), deinitialize);
      });

  per_host_wk_remaining.deinitialize();
}

// #####################################################################
//                        TC GRAPH HBMAINS
// #####################################################################
void HBGraphMDL(pando::Array<char> filename, int64_t num_vertices, TC_CHUNK tc_chunk,
                galois::DAccumulator<uint64_t> final_tri_count) {
#if BENCHMARK
  std::cerr << "MIRRORED DLCSR: num_vertices: " << num_vertices << ", TC_CHUNK: " << tc_chunk
            << "\n";
#endif

  auto thisPlace = pando::getCurrentPlace();
  GraphMDL graph =
      galois::initializeELDLCSR<GraphMDL, MirroredVT, galois::ELEdge>(filename, num_vertices);

  pando::GlobalPtr<GraphMDL> graph_ptr = static_cast<pando::GlobalPtr<GraphMDL>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphMDL)));
  *graph_ptr = graph;

  PANDO_MEM_STAT_NEW_KERNEL("TC_DFS_Algo Start");
  switch (tc_chunk) {
    case TC_CHUNK::CHUNK_VERTICES:
    case TC_CHUNK::CHUNK_EDGES:
      std::cerr << "Chunking not implemented for MDLCSR ... defaulting to no chunk\n";
    default:
      tc_mdlcsr_noChunk(graph_ptr, final_tri_count);
      break;
  }

  graph.deinitialize();
  pando::deallocateMemory(graph_ptr, 1);
}

void HBGraphDL(pando::Array<char> filename, int64_t num_vertices, TC_CHUNK tc_chunk,
               galois::DAccumulator<uint64_t> final_tri_count) {
#if BENCHMARK
  std::cerr << "DLCSR: num_vertices: " << num_vertices << ", TC_CHUNK: " << tc_chunk << "\n";
#endif
  auto thisPlace = pando::getCurrentPlace();

  GraphDL graph =
      galois::initializeELDLCSR<GraphDL, galois::ELVertex, galois::ELEdge>(filename, num_vertices);

  pando::GlobalPtr<GraphDL> graph_ptr = static_cast<pando::GlobalPtr<GraphDL>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDL)));
  *graph_ptr = graph;

  PANDO_MEM_STAT_NEW_KERNEL("TC_DFS_Algo Start");

  switch (tc_chunk) {
    case TC_CHUNK::CHUNK_VERTICES:
      tc_chunk_vertices(graph_ptr, final_tri_count);
      break;
    case TC_CHUNK::CHUNK_EDGES:
      tc_chunk_edges(graph_ptr, final_tri_count);
      break;
    default:
      tc_no_chunk<GraphDL>(graph_ptr, final_tri_count);
      break;
  }

  graph.deinitialize();
  pando::deallocateMemory(graph_ptr, 1);
}

void HBGraphDA(pando::Array<char> filename, int64_t num_vertices,
               galois::DAccumulator<uint64_t> final_tri_count) {
#if BENCHMARK
  std::cerr << "DACSR: num_vertices: " << num_vertices << "\n";
#endif
  auto thisPlace = pando::getCurrentPlace();

  GraphDA graph =
      galois::initializeELDACSR<GraphDA, galois::ELVertex, galois::ELEdge>(filename, num_vertices);

  pando::GlobalPtr<GraphDA> graph_ptr = static_cast<pando::GlobalPtr<GraphDA>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDA)));
  *graph_ptr = graph;

  PANDO_MEM_STAT_NEW_KERNEL("TC_DFS_Algo Start");
  tc_no_chunk<GraphDA>(graph_ptr, final_tri_count);
  graph.deinitialize();
  pando::deallocateMemory(graph_ptr, 1);
}

void HBMainTC(pando::Notification::HandleType hb_done, pando::Array<char> filename,
              int64_t num_vertices, TC_CHUNK tc_chunk, GRAPH_TYPE graph_type,
              galois::DAccumulator<uint64_t> final_tri_count) {
  switch (graph_type) {
    case GRAPH_TYPE::MDLCSR:
      HBGraphMDL(filename, num_vertices, tc_chunk, final_tri_count);
      break;
    case GRAPH_TYPE::DACSR:
      HBGraphDA(filename, num_vertices, final_tri_count);
      break;
    default:
      HBGraphDL(filename, num_vertices, tc_chunk, final_tri_count);
      break;
  }
  hb_done.notify();
}
