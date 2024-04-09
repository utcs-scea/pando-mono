// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <tc_algos.hpp>

// #####################################################################
//                            TC UTILS
// #####################################################################
template <typename Graph>
void edge_tc_counting(pando::GlobalPtr<Graph> graph_ptr, typename Graph::VertexTopologyID v0,
                      typename Graph::EdgeRange edge_range,
                      galois::DAccumulator<uint64_t> final_tri_count) {
  galois::WaitGroup wg;
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
 * @brief Runs Chunked Triangle Counting Algorithm on DistLocalCSRs (GraphDL)
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
void TC_Algo_Chunk_Edges(pando::GlobalPtr<GraphDL> graph_ptr,
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
}

/**
 * @brief Runs the Triangle Counting Algorithm
 *
 * @param[in] graph_ptr Pointer to the in-memory graph
 * @param[in] final_tri_count Thread-safe counter
 */
template <typename GraphType>
void TC_Algo(pando::GlobalPtr<GraphType> graph_ptr,
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
      galois::initializeELDLCSR<galois::ELVertex, galois::ELEdge>(filename, num_vertices);

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
    case TC_CHUNK::CHUNK_EDGES:
      TC_Algo_Chunk_Edges(graph_ptr, final_tri_count);
      break;
    case TC_CHUNK::CHUNK_VERTICES:
      TC_Algo_Chunk_Vertices(graph_ptr, final_tri_count);
      break;
    default:
      TC_Algo<GraphDL>(graph_ptr, final_tri_count);
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
  std::cerr << "HB DA\n";
#if BENCHMARK
  auto time_graph_import_st = std::chrono::high_resolution_clock().now();
#endif

  GraphDA graph =
      galois::initializeELDACSR<galois::ELVertex, galois::ELEdge>(filename, num_vertices);

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
  TC_Algo<GraphDA>(graph_ptr, final_tri_count);
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

void HBMainDFS(pando::Notification::HandleType hb_done, pando::Array<char> filename,
               int64_t num_vertices, bool load_balanced_graph, TC_CHUNK tc_chunk,
               galois::DAccumulator<uint64_t> final_tri_count) {
  auto thisPlace = pando::getCurrentPlace();

  if (load_balanced_graph)
    HBGraphDL(thisPlace, filename, num_vertices, tc_chunk, final_tri_count);
  else
    HBGraphDA(thisPlace, filename, num_vertices, final_tri_count);

  hb_done.notify();
}
