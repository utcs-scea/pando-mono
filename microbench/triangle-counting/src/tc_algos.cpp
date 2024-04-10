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
template <typename GraphType>
void tc_no_chunk(pando::GlobalPtr<GraphType> graph_ptr,
                 galois::DAccumulator<uint64_t> final_tri_count) {
  GraphType graph = *graph_ptr;
  auto state = galois::make_tpl(graph_ptr, final_tri_count);
  galois::doAll(
      state, graph.vertices(), +[](decltype(state) state, typename GraphType::VertexTopologyID v0) {
        auto [graph_ptr, final_tri_count] = state;
        GraphType graph = *graph_ptr;

        // Degree Filtering Optimization
        uint64_t v0_degree = graph.getNumEdges(v0);
        if (v0_degree < (TC_EMBEDDING_SZ - 1))
          return;

        edge_tc_counting<GraphType>(graph_ptr, v0, graph.edges(v0), final_tri_count);
      });
}

void HBGraphDL(pando::Place thisPlace, pando::Array<char> filename, int64_t num_vertices,
               TC_CHUNK tc_chunk, galois::DAccumulator<uint64_t> final_tri_count) {
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
    // case TC_CHUNK::CHUNK_EDGES:
    //   tc_chunk_edges(graph_ptr, final_tri_count);
    //   break;
    // case TC_CHUNK::CHUNK_VERTICES:
    //   tc_chunk_vertices(graph_ptr, final_tri_count);
    //   break;
    default:
      tc_no_chunk<GraphDL>(graph_ptr, final_tri_count);
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
               galois::DAccumulator<uint64_t> final_tri_count) {
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
  tc_no_chunk<GraphDA>(graph_ptr, final_tri_count);
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

void HBMainTC(pando::Notification::HandleType hb_done, pando::Array<char> filename,
              int64_t num_vertices, bool load_balanced_graph, TC_CHUNK tc_chunk,
              galois::DAccumulator<uint64_t> final_tri_count) {
  auto thisPlace = pando::getCurrentPlace();

  if (load_balanced_graph)
    HBGraphDL(thisPlace, filename, num_vertices, tc_chunk, final_tri_count);
  else
    HBGraphDA(thisPlace, filename, num_vertices, final_tri_count);

  hb_done.notify();
}
