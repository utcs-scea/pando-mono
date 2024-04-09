// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <random>

#include "pando-rt/export.h"

#include "pando-lib-galois/loops/do_all.hpp"
#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-wf4-galois/influence_maximization.hpp>

namespace {

const uint64_t num_nodes = 16;
const uint64_t num_edges = 240;
const uint64_t seed = 9801;

wf4::NetworkGraph generateTestGraph(bool set_node_properties = false) {
  wf4::NetworkGraph graph{};
  pando::Vector<wf4::NetworkNode> vertices;
  pando::Vector<galois::GenericEdge<wf4::NetworkEdge>> edges;
  EXPECT_EQ(vertices.initialize(num_nodes), pando::Status::Success);
  EXPECT_EQ(edges.initialize(num_edges), pando::Status::Success);

  for (uint64_t i = 0; i < num_nodes; i++) {
    wf4::NetworkNode node;
    EXPECT_EQ(node.initialize(i), pando::Status::Success);
    if (set_node_properties) {
      *node.sold_ = i * i;
      *node.bought_ = (num_nodes + i) * (num_nodes - (i + 1)) / 2;
      node.desired_ = *node.bought_;
      *node.frequency_ = i;
    }
    vertices[i] = node;
  }
  // Every node sells to every node with a global ID less than itself
  // And so every node buys from every node with a global ID more than itself
  // We set the edge weight to be equal to the global ID of the seller
  // Vertex 0 does not sell anything
  uint64_t edge_count = 0;
  for (uint64_t src = 0; src < num_nodes; src++) {
    for (uint64_t dst = 0; dst < src; dst++) {
      edges[edge_count++] =
          galois::GenericEdge(src, dst, wf4::NetworkEdge(src, agile::TYPES::SALE));
    }
    for (uint64_t dst = src + 1; dst < num_nodes; dst++) {
      edges[edge_count++] =
          galois::GenericEdge(src, dst, wf4::NetworkEdge(dst, agile::TYPES::PURCHASE));
    }
  }

  EXPECT_EQ(graph.initialize(vertices, edges), pando::Status::Success);
  vertices.deinitialize();
  edges.deinitialize();
  return graph;
}

struct DebugState {
  DebugState() = default;
  DebugState(wf4::NetworkGraph graph_, galois::WaitGroup::HandleType handle_,
             galois::DAccumulator<uint64_t> global_nodes_,
             galois::DAccumulator<uint64_t> global_edges_)
      : graph(graph_), handle(handle_), global_nodes(global_nodes_), global_edges(global_edges_) {}
  DebugState(wf4::NetworkGraph graph_, galois::DAccumulator<double> total_edge_weights_,
             galois::DAccumulator<double> total_sales_)
      : graph(graph_), total_edge_weights(total_edge_weights_), total_sales(total_sales_) {}

  wf4::NetworkGraph graph;
  galois::WaitGroup::HandleType handle;

  galois::DAccumulator<uint64_t> global_nodes;
  galois::DAccumulator<uint64_t> global_edges;

  galois::DAccumulator<double> total_edge_weights;
  galois::DAccumulator<double> total_sales;
};

double AmountSold(uint64_t num_nodes) {
  double sold = 0;
  for (uint64_t i = 0; i < num_nodes; i++) {
    sold += i * i;
  }
  return sold;
}

#ifdef DIST_ARRAY_CSR
wf4::NetworkGraph::VertexTokenID getRandomNode(wf4::NetworkGraph& graph,
                                               std::mt19937_64& generator) {
  std::uniform_int_distribution<size_t> dist_nodes =
      std::uniform_int_distribution<size_t>(0, graph.size() - 1);
  return graph.getTokenID(dist_nodes(generator));
}
#else
wf4::NetworkGraph::VertexTokenID getRandomNode(wf4::NetworkGraph& graph,
                                               std::mt19937_64& generator) {
  galois::LCSR<wf4::NetworkNode, wf4::NetworkEdge> g = graph.getLocalCSR();
  std::uniform_int_distribution<size_t> dist_nodes =
      std::uniform_int_distribution<size_t>(0, g.size() - 1);
  return graph.getTokenID(*(g.vertices().begin() + dist_nodes(generator)));
}
#endif

pando::Vector<uint64_t> getRootCounts(wf4::NetworkGraph& graph, uint64_t num_sets) {
  pando::Vector<uint64_t> root_counts;
  EXPECT_EQ(root_counts.initialize(graph.size()), pando::Status::Success);
  for (uint64_t i = 0; i < root_counts.size(); i++) {
    root_counts[i] = 0;
  }
  for (uint64_t i = 0; i < num_sets; i++) {
    std::mt19937_64 generator(seed + i);
    wf4::NetworkGraph::VertexTokenID root = getRandomNode(graph, generator);
    uint64_t old_count = root_counts[root];
    root_counts[root] = old_count + 1;
  }
  return root_counts;
}

} // namespace

TEST(IF, INIT) {
  wf4::NetworkGraph graph = generateTestGraph();
  EXPECT_EQ(graph.size(), num_nodes);
  galois::WaitGroup wg;
  galois::DAccumulator<uint64_t> global_nodes{};
  galois::DAccumulator<uint64_t> global_edges{};
  EXPECT_EQ(wg.initialize(num_nodes + num_edges), pando::Status::Success);
  EXPECT_EQ(global_nodes.initialize(), pando::Status::Success);
  EXPECT_EQ(global_edges.initialize(), pando::Status::Success);
  DebugState state{graph, wg.getHandle(), global_nodes, global_edges};

  galois::doAll(
      state, graph.vertices(),
      +[](DebugState state, typename wf4::NetworkGraph::VertexTopologyID node_lid) {
        wf4::NetworkGraph graph = state.graph;
        bool local = graph.isLocal(node_lid);
        EXPECT_EQ(local, true);
        if (!local) {
          return;
        }
        state.handle.done();
        state.global_nodes.increment();
        uint64_t node_edges = graph.getNumEdges(node_lid);
        EXPECT_EQ(node_edges, num_nodes - 1);

        // TODO(Patrick) when `graph.edges(node_lid)` returns edge handle iterators use doAll
        for (uint64_t edge = 0; edge < node_edges; edge++) {
          wf4::NetworkEdge edge_value = graph.getEdgeData(node_lid, edge);
          wf4::NetworkGraph::VertexTokenID token_id = graph.getTokenID(node_lid);
          EXPECT_EQ(edge_value.type == agile::TYPES::SALE,
                    graph.getTokenID(graph.getEdgeDst(node_lid, edge)) < token_id);
          EXPECT_EQ(edge_value.type == agile::TYPES::PURCHASE,
                    graph.getTokenID(graph.getEdgeDst(node_lid, edge)) > token_id);
          state.handle.done();
          state.global_edges.increment();
        }
      });
  EXPECT_EQ(wg.wait(), pando::Status::Success);
  EXPECT_EQ(global_nodes.reduce(), num_nodes);
  EXPECT_EQ(global_edges.reduce(), num_edges);
  global_edges.deinitialize();
  global_nodes.deinitialize();
  wg.deinitialize();
  graph.deinitialize();
}

TEST(IF, FillNodeValues) {
  wf4::NetworkGraph graph = generateTestGraph();
  galois::doAll(
      graph, graph.vertices(),
      +[](wf4::NetworkGraph graph, typename wf4::NetworkGraph::VertexTopologyID node_lid) {
        wf4::internal::FillNodeValues(graph, node_lid);
        wf4::NetworkNode node_data = graph.getData(node_lid);
        wf4::NetworkGraph::VertexTokenID token_id = graph.getTokenID(node_lid);
        EXPECT_EQ(*node_data.sold_, token_id * token_id);
        EXPECT_EQ(*node_data.bought_, (num_nodes + token_id) * (num_nodes - (token_id + 1)) / 2);
        EXPECT_EQ(*node_data.bought_, node_data.desired_);
      });
  graph.deinitialize();
}

TEST(IF, EdgeProbability) {
  const bool set_node_properties = true;
  wf4::NetworkGraph graph = generateTestGraph(set_node_properties);
  galois::DAccumulator<double> total_edge_weights{};
  galois::DAccumulator<double> total_sales{};
  EXPECT_EQ(total_edge_weights.initialize(), pando::Status::Success);
  EXPECT_EQ(total_sales.initialize(), pando::Status::Success);
  wf4::internal::EdgeProbabilityState state(graph, total_edge_weights, total_sales);
  galois::doAll(
      state, graph.vertices(),
      +[](wf4::internal::EdgeProbabilityState& state,
          typename wf4::NetworkGraph::VertexTopologyID node_lid) {
        wf4::internal::CalculateEdgeProbability(state, node_lid);
        uint64_t node_edges = state.graph.getNumEdges(node_lid);
        for (uint64_t edge = 0; edge < node_edges; edge++) {
          wf4::NetworkEdge edge_value = state.graph.getEdgeData(node_lid, edge);
          if (edge_value.type == agile::TYPES::SALE) {
            EXPECT_FLOAT_EQ(edge_value.weight_, 1.0 / state.graph.getTokenID(node_lid));
          } else {
            EXPECT_FLOAT_EQ(edge_value.weight_,
                            1.0 / state.graph.getTokenID(state.graph.getEdgeDst(node_lid, edge)));
          }
        }
      });
  EXPECT_FLOAT_EQ(total_edge_weights.reduce(), (num_nodes - 1) * 2);
  EXPECT_FLOAT_EQ(total_sales.reduce(), AmountSold(num_nodes) * 2);
  total_sales.deinitialize();
  total_edge_weights.deinitialize();
  graph.deinitialize();
}

TEST(IF, EdgeProbabilities) {
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  galois::doAll(
      graph, graph.vertices(),
      +[](wf4::NetworkGraph graph, typename wf4::NetworkGraph::VertexTopologyID node_lid) {
        wf4::NetworkNode node_data = graph.getData(node_lid);
        wf4::NetworkGraph::VertexTokenID token_id = graph.getTokenID(node_lid);
        EXPECT_EQ(*node_data.sold_, token_id * token_id);
        EXPECT_EQ(*node_data.bought_, (num_nodes + token_id) * (num_nodes - (token_id + 1)) / 2);
        EXPECT_EQ(*node_data.bought_, node_data.desired_);
      });
  galois::doAll(
      graph, graph.vertices(),
      +[](wf4::NetworkGraph graph, typename wf4::NetworkGraph::VertexTopologyID node_lid) {
        uint64_t node_edges = graph.getNumEdges(node_lid);
        for (uint64_t edge = 0; edge < node_edges; edge++) {
          wf4::NetworkEdge edge_value = graph.getEdgeData(node_lid, edge);
          if (edge_value.type == agile::TYPES::SALE) {
            EXPECT_FLOAT_EQ(edge_value.weight_, 1.0 / graph.getTokenID(node_lid));
          } else {
            EXPECT_FLOAT_EQ(edge_value.weight_,
                            1.0 / graph.getTokenID(graph.getEdgeDst(node_lid, edge)));
          }
        }
      });
  graph.deinitialize();
}

TEST(IF, GenerateRRRSet) {
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  wf4::ReverseReachableSet rrr_sets = wf4::GetRandomReverseReachableSets(graph, 1, seed);
  std::mt19937_64 generator(seed);
  wf4::NetworkGraph::VertexTokenID root = getRandomNode(graph, generator);

  EXPECT_EQ(rrr_sets.sizeAll(), 1);
  bool has_nonempty = false;
  for (pando::Vector<pando::Vector<wf4::NetworkGraph::VertexTokenID>> vec : rrr_sets) {
    for (pando::Vector<wf4::NetworkGraph::VertexTokenID> rrr_set : vec) {
      EXPECT_GT(rrr_set.size(), 0);
      wf4::NetworkGraph::VertexTokenID walked_root = rrr_set[0];
      EXPECT_EQ(walked_root, root);
      has_nonempty = true;
    }
  }
  EXPECT_EQ(has_nonempty, true);
  wf4::NetworkNode root_data = graph.getData(graph.getTopologyID(root));
  uint64_t root_influence = *root_data.frequency_;
  EXPECT_EQ(root_influence, 1);

  rrr_sets.deinitialize();
  graph.deinitialize();
}

TEST(IF, GenerateRRRSets) {
  const uint64_t num_sets = 100;
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  wf4::ReverseReachableSet rrr_sets = wf4::GetRandomReverseReachableSets(graph, num_sets, seed);
  pando::Vector<uint64_t> root_counts = getRootCounts(graph, num_sets);

  EXPECT_EQ(rrr_sets.sizeAll(), num_sets);
  bool has_nonempty = false;
  for (pando::Vector<pando::Vector<wf4::NetworkGraph::VertexTokenID>> vec : rrr_sets) {
    for (pando::Vector<wf4::NetworkGraph::VertexTokenID> rrr_set : vec) {
      EXPECT_GT(rrr_set.size(), 0);
      has_nonempty = true;
    }
  }
  EXPECT_EQ(has_nonempty, true);

  bool has_greater = false;
  for (uint64_t i = 0; i < num_nodes; i++) {
    wf4::NetworkNode node_data = graph.getData(graph.getTopologyID(i));
    uint64_t influence = *node_data.frequency_;
    uint64_t root_occurrences = root_counts[i];
    EXPECT_GE(influence, root_occurrences);
    has_greater |= influence > root_occurrences;
  }
  EXPECT_EQ(has_greater, true);

  rrr_sets.deinitialize();
  graph.deinitialize();
}

TEST(IF, FindLocalMax) {
  wf4::NetworkGraph graph = generateTestGraph(true);
  galois::PerThreadVector<wf4::internal::LocalMaxNode> max_array{};
  galois::DAccumulator<uint64_t> total_influence{};
  EXPECT_EQ(max_array.initialize(), pando::Status::Success);
  EXPECT_EQ(total_influence.initialize(), pando::Status::Success);

  wf4::internal::MaxState state(graph, max_array, total_influence);
  galois::doAll(state, graph.vertices(), &wf4::internal::FindLocalMaxNode);
  EXPECT_EQ(total_influence.reduce(), num_edges / 2);
  EXPECT_GT(max_array.sizeAll(), 0);

  total_influence.deinitialize();
  max_array.deinitialize();
  graph.deinitialize();
}

TEST(IF, GetMaxNode) {
  wf4::NetworkGraph graph = generateTestGraph(true);
  EXPECT_EQ(wf4::internal::GetMostInfluentialNode(graph, 1), num_nodes - 1);
  graph.deinitialize();
}

TEST(IF, GetInfluential) {
  const uint64_t num_sets = 100;
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  wf4::ReverseReachableSet rrr_sets = wf4::GetRandomReverseReachableSets(graph, num_sets, seed);
  pando::Vector<wf4::NetworkGraph::VertexTokenID> influential_nodes =
      wf4::GetInfluentialNodes(graph, std::move(rrr_sets), 1);
  EXPECT_EQ(influential_nodes.size(), 1);
  wf4::NetworkGraph::VertexTokenID influential = influential_nodes[0];
  wf4::NetworkNode most_influential = graph.getData(graph.getTopologyID(influential));
  uint64_t most_influence = *most_influential.frequency_;
  EXPECT_GT(most_influence, 0);
  for (uint64_t node = 0; node < num_nodes; node++) {
    wf4::NetworkNode node_data = graph.getData(graph.getTopologyID(node));
    uint64_t influence = *node_data.frequency_;
    EXPECT_GE(most_influence, influence);
  }
  graph.deinitialize();
}

TEST(IF, GetInfluentials2) {
  const uint64_t num_sets = 100;
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  wf4::ReverseReachableSet rrr_sets = wf4::GetRandomReverseReachableSets(graph, num_sets, seed);
  pando::Vector<wf4::NetworkGraph::VertexTokenID> influential_nodes =
      wf4::GetInfluentialNodes(graph, std::move(rrr_sets), 2);
  EXPECT_EQ(influential_nodes.size(), 2);
  wf4::NetworkGraph::VertexTokenID influential = influential_nodes[0];
  wf4::NetworkNode most_influential = graph.getData(graph.getTopologyID(influential));
  uint64_t most_influence = *most_influential.frequency_;
  EXPECT_EQ(most_influence, 0);
  graph.deinitialize();
}

TEST(IF, GetInfluentials3) {
  const uint64_t num_sets = 100;
  wf4::NetworkGraph graph = generateTestGraph();
  wf4::CalculateEdgeProbabilities(graph);
  wf4::ReverseReachableSet rrr_sets = wf4::GetRandomReverseReachableSets(graph, num_sets, seed);
  pando::Vector<wf4::NetworkGraph::VertexTokenID> influential_nodes =
      wf4::GetInfluentialNodes(graph, std::move(rrr_sets), 3);
  EXPECT_EQ(influential_nodes.size(), 3);
  wf4::NetworkGraph::VertexTokenID influential = influential_nodes[0];
  wf4::NetworkNode most_influential = graph.getData(graph.getTopologyID(influential));
  uint64_t most_influence = *most_influential.frequency_;
  EXPECT_EQ(most_influence, 0);
  for (uint64_t node = 0; node < num_nodes; node++) {
    wf4::NetworkNode node_data = graph.getData(graph.getTopologyID(node));
    uint64_t influence = *node_data.frequency_;
    EXPECT_LE(influence, 100);
  }
  graph.deinitialize();
}
