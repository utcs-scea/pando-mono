// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <tc_algos.hpp>
#include <type_traits>

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
template <typename Graph, bool binary_search>
void edge_tc_counting(galois::WaitGroup::HandleType wgh, pando::GlobalPtr<Graph> graph_ptr,
                      typename Graph::VertexTopologyID v0, typename Graph::EdgeRange edge_range,
                      galois::DAccumulator<uint64_t> final_tri_count) {
  auto innerState = galois::make_tpl(graph_ptr, v0, wgh, final_tri_count);
  Graph graph = *graph_ptr;
  galois::doAll(
      wgh, innerState, edge_range,
      +[](decltype(innerState) innerState, typename Graph::EdgeHandle eh) {
        auto [graph_ptr, v0, wgh, final_tri_count] = innerState;
        Graph g = *graph_ptr;
        typename Graph::VertexTopologyID v1 = apply(g, getEdgeDst, eh);
        if (binary_search)
          intersect_dag_merge_double_binary<Graph>(graph_ptr, v0, v1, final_tri_count);
        else
          intersect_dag_merge<Graph>(graph_ptr, v0, v1, final_tri_count);
      },
      [&graph](decltype(innerState) innerState, typename Graph::EdgeHandle eh) -> pando::Place {
        auto v0 = std::get<1>(innerState);
        typename Graph::VertexTopologyID v1 = apply(graph, getEdgeDst, eh);
        bool v0_higher_degree = apply(graph, getNumEdges, v0) >= apply(graph, getNumEdges, v1);
        pando::Place locality = v0_higher_degree ? apply(graph, getLocalityVertex, v0)
                                                 : apply(graph, getLocalityVertex, v1);
        return locality;
      });
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
template <typename GraphType, bool binary_search>
void tc_no_chunk(pando::GlobalPtr<GraphType> graph_ptr,
                 galois::DAccumulator<uint64_t> final_tri_count) {
  GraphType graph = *graph_ptr;

  galois::WaitGroup wg;
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();
  auto state = galois::make_tpl(graph_ptr, final_tri_count, wgh);

  galois::doAll(
      wgh, state, graph.vertices(),
      +[](decltype(state) state, typename GraphType::VertexTopologyID v0) {
        auto [graph_ptr, final_tri_count, wgh] = state;
        GraphType graph = *graph_ptr;

        // Degree Filtering Optimization
        uint64_t v0_degree = graph.getNumEdges(v0);
        if (v0_degree < (TC_EMBEDDING_SZ - 1))
          return;

        edge_tc_counting<GraphType, binary_search>(wgh, graph_ptr, v0, graph.edges(v0),
                                                   final_tri_count);
      });
  PANDO_CHECK(wg.wait());
  wg.deinitialize();
}

/**
 * @brief Runs Chunked Edges Triangle Counting Algorithm on DistLocalCSRs (GraphDL)
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
/**
void tc_chunk_edges(pando::GlobalPtr<GraphDL> graph_ptr,
                    galois::DAccumulator<uint64_t> final_tri_count) {
  GraphDL graph = *graph_ptr;
  uint64_t query_sz = 1;
  uint64_t iters = 0;

  galois::DAccumulator<uint64_t> work_remaining{};
  PANDO_CHECK(work_remaining.initialize());

  // Assumption: ELVertex iterator_offset = 0
  do {
    work_remaining.reset();
    auto state = galois::make_tpl(graph_ptr, work_remaining, query_sz, final_tri_count);
    galois::doAll(
        state, graph.vertices(), +[](decltype(state) state, typename GraphDL::VertexTopologyID v0) {
          auto [graph_ptr, work_remaining, query_sz, final_tri_count] = state;
          GraphDL g = *graph_ptr;

          // DF Optimization
          uint64_t v0_numEdges = fmap(g, getNumEdges, v0);
          if (v0_numEdges < (TC_EMBEDDING_SZ - 1))
            return;

          // Each vertex stores a bookmark where they left off
          // We modify DistLocalCSR (GraphDL) to allow us to capture a subset of the edgeRange
          uint64_t current_offset = v0->iterator_offset;
          auto curr_edge_range = fmap(g, edges, v0, current_offset, query_sz);
          auto local_work_remaining = curr_edge_range.size();

          edge_tc_counting<GraphDL>(graph_ptr, v0, curr_edge_range, final_tri_count);

          // Move bookmark forward and flag if this vertex is not yet done
          v0->iterator_offset += local_work_remaining;
          if (v0->iterator_offset < v0_numEdges)
            work_remaining.increment();
        });

    uint64_t current_count = final_tri_count.reduce();
    std::cout << "After Iter " << iters << " found " << current_count << " triangles.\n";
    final_tri_count.reset();
    final_tri_count.add(current_count);
    iters++;
    query_sz <<= 1;
  } while (work_remaining.reduce());
  work_remaining.deinitialize();
}
*/

/**
 * @brief Runs Chunked Vertices Triangle Counting Algorithm on DistLocalCSRs (GraphDL)
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <bool binary_search>
void tc_chunk_vertices(pando::GlobalPtr<GraphDL> graph_ptr,
                       galois::DAccumulator<uint64_t> final_tri_count) {
  using LCSR = galois::LCSR<VT, ET>;

  GraphDL graph = *graph_ptr;
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
          GraphDL graph = *graph_ptr;
          auto lcsr = graph.getLocalCSR();
          uint64_t host_vertex_iter_offset = host_vertex_iter_offset_ref;

          galois::WaitGroup wg;
          PANDO_CHECK(wg.initialize(0));
          auto wgh = wg.getHandle();
          auto inner_state = galois::make_tpl(graph_ptr, final_tri_count, wgh);
          galois::doAll(
              inner_state, apply(lcsr, vertices, host_vertex_iter_offset, query_sz),
              +[](decltype(inner_state) inner_state, typename GraphDL::VertexTopologyID v0) {
                auto [graph_ptr, final_tri_count, wgh] = inner_state;
                GraphDL graph = *graph_ptr;

                // Degree Filtering Optimization
                uint64_t v0_degree = graph.getNumEdges(v0);
                if (v0_degree < (TC_EMBEDDING_SZ - 1))
                  return;

                edge_tc_counting<GraphDL, binary_search>(wgh, graph_ptr, v0, graph.edges(v0),
                                                         final_tri_count);
              });
          PANDO_CHECK(wg.wait());

          // Move iter offset
          uint64_t lcsr_num_vertices = apply(lcsr, size);
          host_vertex_iter_offset += query_sz;
          if (host_vertex_iter_offset < lcsr_num_vertices)
            work_remaining.increment();
          host_vertex_iter_offset_ref = host_vertex_iter_offset;
          wg.deinitialize();
        });

    uint64_t current_count = final_tri_count.reduce();
    std::cout << "After Iter " << iters << " found " << current_count << " triangles.\n";
    final_tri_count.reset();
    final_tri_count.add(current_count);
    iters++;
    query_sz <<= 1;
  } while (work_remaining.reduce());

  work_remaining.deinitialize();
  per_host_iterator_offsets.deinitialize();
}

// #####################################################################
//                        TC GRAPH HBMAINS
// #####################################################################
void HBGraphDL(pando::Place thisPlace, pando::Array<char> filename, int64_t num_vertices,
               TC_CHUNK tc_chunk, bool binary_search,
               galois::DAccumulator<uint64_t> final_tri_count) {
#if BENCHMARK
  auto time_graph_import_st = std::chrono::high_resolution_clock().now();
#endif
  GraphDL graph =
      galois::initializeELDLCSR<GraphDL, galois::ELVertex, galois::ELEdge>(filename, num_vertices);

#if BENCHMARK
  auto time_graph_import_end = std::chrono::high_resolution_clock().now();
  if (thisPlace.node.id == COORDINATOR_ID) {
    std::cout << "Time_Graph_Creation(ms), "
              << std::chrono::duration_cast<std::chrono::milliseconds>(time_graph_import_end -
                                                                       time_graph_import_st)
                     .count()
              << "\n";
  }
#endif

  pando::GlobalPtr<GraphDL> graph_ptr = static_cast<pando::GlobalPtr<GraphDL>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDL)));
  *graph_ptr = graph;

#if BENCHMARK
  auto time_tc_algo_st = std::chrono::high_resolution_clock().now();
#endif
  PANDO_MEM_STAT_NEW_KERNEL("TC_DFS_Algo Start");

  switch (tc_chunk) {
    case TC_CHUNK::CHUNK_VERTICES:
      if (binary_search)
        tc_chunk_vertices<true>(graph_ptr, final_tri_count);
      else
        tc_chunk_vertices<false>(graph_ptr, final_tri_count);
      break;
    /**
    case TC_CHUNK::CHUNK_EDGES:
      tc_chunk_edges(graph_ptr, final_tri_count);
      break;
    */
    default:
      if (binary_search)
        tc_no_chunk<GraphDL, true>(graph_ptr, final_tri_count);
      else
        tc_no_chunk<GraphDL, false>(graph_ptr, final_tri_count);
      break;
  }

#if BENCHMARK
  auto time_tc_algo_end = std::chrono::high_resolution_clock().now();
  if (thisPlace.node.id == COORDINATOR_ID)
    std::cout << "Time_TC_Algo(ms), "
              << std::chrono::duration_cast<std::chrono::milliseconds>(time_tc_algo_end -
                                                                       time_tc_algo_st)
                     .count()
              << "\n";
#endif
  graph.deinitialize();
  pando::deallocateMemory(graph_ptr, 1);
}

void HBGraphDA(pando::Place thisPlace, pando::Array<char> filename, int64_t num_vertices,
               bool binary_search, galois::DAccumulator<uint64_t> final_tri_count) {
#if BENCHMARK
  auto time_graph_import_st = std::chrono::high_resolution_clock().now();
#endif

  GraphDA graph =
      galois::initializeELDACSR<GraphDA, galois::ELVertex, galois::ELEdge>(filename, num_vertices);

#if BENCHMARK
  auto time_graph_import_end = std::chrono::high_resolution_clock().now();
  if (thisPlace.node.id == COORDINATOR_ID) {
    std::cout << "Time_Graph_Creation(ms), "
              << std::chrono::duration_cast<std::chrono::milliseconds>(time_graph_import_end -
                                                                       time_graph_import_st)
                     .count()
              << "\n";
  }
#endif

  pando::GlobalPtr<GraphDA> graph_ptr = static_cast<pando::GlobalPtr<GraphDA>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDA)));
  *graph_ptr = graph;

#if BENCHMARK
  auto time_tc_algo_st = std::chrono::high_resolution_clock().now();
#endif
  PANDO_MEM_STAT_NEW_KERNEL("TC_DFS_Algo Start");
  if (binary_search)
    tc_no_chunk<GraphDA, true>(graph_ptr, final_tri_count);
  else
    tc_no_chunk<GraphDA, false>(graph_ptr, final_tri_count);
#if BENCHMARK
  auto time_tc_algo_end = std::chrono::high_resolution_clock().now();
  if (thisPlace.node.id == COORDINATOR_ID)
    std::cout << "Time_TC_Algo(ms), "
              << std::chrono::duration_cast<std::chrono::milliseconds>(time_tc_algo_end -
                                                                       time_tc_algo_st)
                     .count()
              << "\n";
#endif
  graph.deinitialize();
  pando::deallocateMemory(graph_ptr, 1);
}

void HBMainTC(pando::Array<char> filename, int64_t num_vertices, bool load_balanced_graph,
              TC_CHUNK tc_chunk, bool binary_search,
              galois::DAccumulator<uint64_t> final_tri_count) {
  auto thisPlace = pando::getCurrentPlace();

  if (load_balanced_graph)
    HBGraphDL(thisPlace, filename, num_vertices, tc_chunk, binary_search, final_tri_count);
  else
    HBGraphDA(thisPlace, filename, num_vertices, binary_search, final_tri_count);
}
