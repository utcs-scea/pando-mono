// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_UTILS_HPP_
#define PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_UTILS_HPP_

#include <getopt.h>
#include <pando-rt/export.h>

#include <memory>
#include <string>
#include <utility>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/prefix_sum.hpp>
#include <pando-lib-galois/utility/tuple.hpp>

#define COORDINATOR_ID    0
#define DEBUG             0
#define BENCHMARK         1
#define SORTED_EDGES      1
#define TC_EMBEDDING_SZ   3
#define OVERDECOMPOSITION 0

using ET = galois::ELEdge;
using VT = galois::ELVertex;
using GraphDL = galois::DistLocalCSR<VT, ET>;
using GraphDA = galois::DistArrayCSR<VT, ET>;

enum TC_CHUNK { NO_CHUNK = 0, CHUNK_EDGES = 1, CHUNK_VERTICES = 2 };

struct CommandLineOptions {
  std::string elFile;
  int64_t num_vertices = 0;
  bool load_balanced_graph = false;
  TC_CHUNK tc_chunk = TC_CHUNK::NO_CHUNK;

  void print() {
    printf("******** CommandLineOptions ******** \n");
    std::cout << "elFile = " << elFile << '\n';
    std::cout << "num_vertices = " << num_vertices << '\n';
    std::cout << "load_balanced_graph = " << load_balanced_graph << '\n';
    std::cout << "tc_chunk = " << tc_chunk << '\n';
    printf("******** END CommandLineOptions ******** \n");
  }

  CommandLineOptions()
      : elFile(""), num_vertices(0), load_balanced_graph(false), tc_chunk(TC_CHUNK::NO_CHUNK) {}
};

std::shared_ptr<CommandLineOptions> read_cmd_line_args(int argc, char** argv);
void printUsageExit(char* argv0);
void printUsage(char* argv0);
// #####################################################################
//                        CONNECTION KERNELS
// #####################################################################
template <typename GraphType>
void intersect_dag_merge(galois::WaitGroup::HandleType wgh, pando::GlobalPtr<GraphType> graph_ptr,
                         typename GraphType::VertexTopologyID v0,
                         typename GraphType::VertexTopologyID v1,
                         galois::DAccumulator<uint64_t> final_tri_count) {
  uint64_t count = 0;
  GraphType graph = *graph_ptr;
  auto p_start = graph.edges(v0).begin();
  auto p_end = graph.edges(v0).end();
  auto q_start = graph.edges(v1).begin();
  auto q_end = graph.edges(v1).end();
  auto p_it = p_start;
  auto q_it = q_start;
  typename GraphType::VertexTokenID a;
  typename GraphType::VertexTokenID b;
  while (p_it < p_end && q_it < q_end) {
    typename GraphType::VertexTopologyID a_loc = graph.getEdgeDst(*p_it);
    typename GraphType::VertexTopologyID b_loc = graph.getEdgeDst(*q_it);
    a = graph.getTokenID(a_loc);
    b = graph.getTokenID(b_loc);
    if (a <= b)
      p_it++;
    if (a >= b)
      q_it++;
    if (a == b)
      count++;
  }
  final_tri_count.add(count);
  wgh.done();
}

// #####################################################################
//                        CONNECTION KERNELS
// #####################################################################
template <typename GraphType>
void is_connected(pando::GlobalPtr<GraphType> graph_ptr,
                  typename GraphType::VertexTokenID neighbor_of_v0_to_find,
                  typename GraphType::VertexTopologyID v1_where_to_find,
                  galois::DAccumulator<uint64_t> final_tri_count) {
  GraphType graph = *graph_ptr;
  for (typename GraphType::EdgeHandle eh_v1 : graph.edges(v1_where_to_find)) {
    typename GraphType::VertexTokenID ehv1_token = graph.getTokenID(graph.getEdgeDst(eh_v1));
    if (ehv1_token == neighbor_of_v0_to_find) {
      final_tri_count.increment();
      return;
    }
#if SORTED_EDGES
    if (ehv1_token > neighbor_of_v0_to_find) {
      break;
    }
#endif
  }
}

template <typename GraphType>
void is_connected_binarySearch(pando::GlobalPtr<GraphType> graph_ptr,
                               typename GraphType::VertexTokenID neighbor_of_v0_to_find,
                               typename GraphType::VertexTopologyID v1_where_to_find,
                               galois::DAccumulator<uint64_t> final_tri_count) {
  GraphType graph = *graph_ptr;
  auto v1_edges = graph.edges(v1_where_to_find);
  int64_t lo = 0;
  int64_t hi = v1_edges.size();

  while (lo < hi) {
    uint64_t edge_indx = (lo + hi) / 2;
    typename GraphType::VertexTopologyID edge_dst = graph.getEdgeDst(v1_where_to_find, edge_indx);
    typename GraphType::VertexTokenID edge_dst_token = graph.getTokenID(edge_dst);
    if (edge_dst_token == neighbor_of_v0_to_find) {
      final_tri_count.increment();
      return;
    }
    if (edge_dst_token < neighbor_of_v0_to_find)
      lo++;
    else
      hi--;
  }
}

template <typename GraphType>
void vertexset_intersection(pando::GlobalPtr<GraphType> graph_ptr,
                            typename GraphType::VertexTopologyID v0,
                            typename GraphType::EdgeHandle eh,
                            galois::DAccumulator<uint64_t> final_tri_count) {
  GraphType graph = *graph_ptr;
  typename GraphType::VertexTopologyID v1 = graph.getEdgeDst(eh);
  typename GraphType::VertexTokenID v1_token = graph.getTokenID(v1);
  auto connection_kernel =
      SORTED_EDGES ? is_connected_binarySearch<GraphType> : is_connected<GraphType>;

#if OVERDECOMPOSITION
  auto state = galois::make_tpl(graph_ptr, v1, final_tri_count, connection_kernel, v1_token);
  auto locality_fn = +[](decltype(state) state, typename GraphType::EdgeHandle eh) {
    auto [graph_ptr, v1, final_tri_count, connection_kernel, v1_token] = state;
    GraphType g = *graph_ptr;
    (void)eh; // Required to prevent -Werror=unused-parameter
    return fmap(g, getLocalityVertex, v1);
  };

  galois::doAll(
      state, graph.edges(v0),
      +[](decltype(state) state, typename GraphType::EdgeHandle eh) {
        auto [graph_ptr, v1, final_tri_count, connection_kernel, v1_token] = state;
        GraphType g = *graph_ptr;
        typename GraphType::VertexTopologyID neighbor_of_v0 = fmap(g, getEdgeDst, eh);
        typename GraphType::VertexTokenID neighbor_of_v0_token =
            fmap(g, getTokenID, neighbor_of_v0);

        // Because of DAG optimization
        if (neighbor_of_v0_token <= v1_token)
          return;
        connection_kernel(graph_ptr, neighbor_of_v0_token, v1, final_tri_count);
      },
      locality_fn);

#else
  for (typename GraphType::EdgeHandle eh : graph.edges(v0)) {
    typename GraphType::VertexTopologyID neighbor_of_v0 = graph.getEdgeDst(eh);
    typename GraphType::VertexTokenID neighbor_of_v0_token = graph.getTokenID(neighbor_of_v0);
    // Because of DAG optimization
    if (neighbor_of_v0_token <= v1_token)
      continue;
    connection_kernel(graph_ptr, neighbor_of_v0_token, v1, final_tri_count);
  }
#endif
}

#endif // PANDO_MONO_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_UTILS_HPP_
