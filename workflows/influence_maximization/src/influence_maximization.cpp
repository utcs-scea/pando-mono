// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-wf4-galois/influence_maximization.hpp"

#include <random>

#include <pando-lib-galois/containers/stack.hpp>

namespace {

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

// TODO(Patrick) parallelize this per host and then reduce
wf4::internal::LocalMaxNode getMaxNode(
    galois::PerThreadVector<wf4::internal::LocalMaxNode> max_array) {
  wf4::internal::LocalMaxNode max;
  for (pando::Vector<wf4::internal::LocalMaxNode> local_maxes : max_array) {
    // local_maxes is a vector of length 0 or 1
    for (wf4::internal::LocalMaxNode local_max : local_maxes) {
      if (local_max.max_influence > max.max_influence) {
        max = local_max;
      }
    }
  }
  return max;
}

void printInfluentialNode(wf4::internal::MaxState& state, wf4::internal::LocalMaxNode node,
                          uint64_t rank) {
  wf4::NetworkNode node_data = state.graph.getData(state.graph.getTopologyID(node.max_node));
  uint64_t num_edges = state.graph.getNumEdges(state.graph.getTopologyID(node.max_node));
  uint64_t host = state.graph.getLocalityVertex(state.graph.getTopologyID(node.max_node)).node.id;
  double bought = *node_data.bought_;
  double sold = *node_data.sold_;
  std::printf(
      "Most influential node %lu on %lu: %lu, Occurred: %lu, Degree: %lu, Bought: %lf, Sold: %lf, "
      "Total Influence in Graph: %lu\n",
      rank, host, node_data.id, node.max_influence, num_edges, bought, sold,
      state.total_influence.reduce());
}

template <typename T>
T vectorContains(const pando::Vector<T>& vec, T elt) {
  for (const T& vec_elt : vec) {
    if (vec_elt == elt) {
      return true;
    }
  }
  return false;
}

} // namespace

void wf4::CalculateEdgeProbabilities(wf4::NetworkGraph& graph) {
  galois::doAll(graph, graph.vertices(), &wf4::internal::FillNodeValues);

  galois::DAccumulator<double> total_edge_weights{};
  galois::DAccumulator<double> total_sales{};
  PANDO_CHECK(total_edge_weights.initialize());
  PANDO_CHECK(total_sales.initialize());

  internal::EdgeProbabilityState state(graph, total_edge_weights, total_sales);
  galois::doAll(state, graph.vertices(), &wf4::internal::CalculateEdgeProbability);

  std::printf("Total Edge weights: %lf\n", total_edge_weights.reduce());
  std::printf("Total sold: %lf\n", total_sales.reduce());
  total_edge_weights.deinitialize();
  total_sales.deinitialize();
}

wf4::ReverseReachableSet wf4::GetRandomReverseReachableSets(wf4::NetworkGraph& graph,
                                                            uint64_t num_sets, uint64_t seed) {
  ReverseReachableSet rrr_sets;
  PANDO_CHECK(rrr_sets.initialize());

  internal::RRRState state(graph, rrr_sets, seed);
  galois::doAllEvenlyPartition(state, num_sets, &internal::GenerateRandomReversibleReachableSet);
  return rrr_sets;
}

pando::Vector<wf4::NetworkGraph::VertexTokenID> wf4::GetInfluentialNodes(
    wf4::NetworkGraph& graph, wf4::ReverseReachableSet&& reachability_sets, uint64_t num_nodes) {
  pando::Vector<wf4::NetworkGraph::VertexTokenID> influential_nodes;
  PANDO_CHECK(influential_nodes.initialize(num_nodes));

  NetworkGraph::VertexTokenID influential_node = internal::GetMostInfluentialNode(graph, 1);
  influential_nodes[0] = influential_node;
  for (uint64_t i = 1; i < num_nodes; i++) {
    internal::RemoveReachableSetsWithInfluentialNode(graph, reachability_sets, influential_node);
    influential_node = internal::GetMostInfluentialNode(graph, i + 1);
    influential_nodes[i] = influential_node;
  }
  // TODO(Patrick) remove debug statement
  uint64_t uninfluenced_sets = 0;
  for (pando::Vector<pando::Vector<wf4::NetworkGraph::VertexTokenID>> vec : reachability_sets) {
    for (pando::Vector<wf4::NetworkGraph::VertexTokenID> rrr_set : vec) {
      if (rrr_set.size() > 0) {
        uninfluenced_sets += 1;
      }
    }
  }
  std::printf("Remaining uninfluenced sets: %lu\n", uninfluenced_sets);
  reachability_sets.deinitialize();
  return influential_nodes;
}

void wf4::internal::RemoveReachableSetWithInfluentialNode(
    wf4::internal::InfluentialState& state,
    pando::GlobalRef<pando::Vector<wf4::NetworkGraph::VertexTokenID>> reachability_set_ref) {
  pando::Vector<wf4::NetworkGraph::VertexTokenID> reachability_set = reachability_set_ref;
  if (vectorContains(reachability_set, state.influential_node)) {
    for (wf4::NetworkGraph::VertexTokenID reachable_node_gid : reachability_set) {
      wf4::NetworkNode node_data =
          state.graph.getData(state.graph.getTopologyID(reachable_node_gid));
      pando::atomicDecrement(node_data.frequency_, 1, std::memory_order::memory_order_relaxed);
    }
    reachability_set.deinitialize();
    reachability_set_ref = reachability_set;
  }
}

void wf4::internal::RemoveReachableSetsWithInfluentialNode(
    wf4::NetworkGraph& graph, wf4::ReverseReachableSet& reachability_sets,
    wf4::NetworkGraph::VertexTokenID influential_node) {
  galois::WaitGroup wg;
  PANDO_CHECK(wg.initialize(0));
  InfluentialState state(graph, influential_node, wg.getHandle());
  galois::doAll(
      wg.getHandle(), state, reachability_sets,
      +[](InfluentialState state,
          pando::GlobalRef<pando::Vector<pando::Vector<wf4::NetworkGraph::VertexTokenID>>> vec) {
        pando::Vector<pando::Vector<wf4::NetworkGraph::VertexTokenID>> v = vec;
        galois::doAll(state.wgh, state, v, &RemoveReachableSetWithInfluentialNode);
      });
  PANDO_CHECK(wg.wait());
}

void wf4::internal::FindLocalMaxNode(wf4::internal::MaxState& state,
                                     wf4::NetworkGraph::VertexTopologyID node) {
  wf4::NetworkNode node_data = state.graph.getData(node);
  uint64_t influence = *node_data.frequency_;
  state.total_influence.add(influence);
  pando::Vector<LocalMaxNode> local_vec = state.max_array.getThreadVector();
  if (local_vec.size() == 0) {
    PANDO_CHECK(state.max_array.pushBack(LocalMaxNode(state.graph.getTokenID(node), influence)));
  } else {
    LocalMaxNode max = local_vec[0];
    if (influence > max.max_influence) {
      local_vec[0] = LocalMaxNode(state.graph.getTokenID(node), influence);
      state.max_array.set(local_vec);
    }
  }
}

wf4::NetworkGraph::VertexTokenID wf4::internal::GetMostInfluentialNode(wf4::NetworkGraph& graph,
                                                                       uint64_t rank) {
  galois::PerThreadVector<wf4::internal::LocalMaxNode> max_array;
  galois::DAccumulator<uint64_t> total_influence{};
  PANDO_CHECK(max_array.initialize());
  PANDO_CHECK(total_influence.initialize());

  MaxState state(graph, max_array, total_influence);
  galois::doAll(state, graph.vertices(), &FindLocalMaxNode);
  wf4::internal::LocalMaxNode max_node = getMaxNode(max_array);

  printInfluentialNode(state, max_node, rank);
  max_array.deinitialize();
  total_influence.deinitialize();
  return max_node.max_node;
}

void wf4::internal::GenerateRandomReversibleReachableSet(wf4::internal::RRRState& state,
                                                         uint64_t set_id, uint64_t) {
  std::mt19937_64 generator(state.seed + set_id);
  std::uniform_real_distribution<double> dist_bfs = std::uniform_real_distribution<double>(0, 1);
  pando::Vector<wf4::NetworkGraph::VertexTokenID> reachable_set;
  galois::Stack<wf4::NetworkGraph::VertexTokenID> frontier;
  wf4::NetworkGraph::VertexTokenID root = getRandomNode(state.graph, generator);
  PANDO_CHECK(reachable_set.initialize(1));
  PANDO_CHECK(frontier.initialize(1));
  reachable_set[0] = root;
  PANDO_CHECK(frontier.emplace(root));

  while (!frontier.empty()) {
    wf4::NetworkGraph::VertexTokenID node_gid;
    PANDO_CHECK(frontier.pop(node_gid));
    wf4::NetworkGraph::VertexTopologyID node_lid = state.graph.getTopologyID(node_gid);
    wf4::NetworkNode node = state.graph.getData(node_lid);
    pando::atomicIncrement(node.frequency_, 1, std::memory_order::memory_order_relaxed);
    for (auto edge : state.graph.edges(node_lid)) {
      wf4::NetworkEdge edge_data = state.graph.getEdgeData(edge);
      if (dist_bfs(generator) <= edge_data.weight_) {
        wf4::NetworkGraph::VertexTokenID reachable_node_gid =
            state.graph.getTokenID(state.graph.getEdgeDst(edge));
        if (!vectorContains(reachable_set, reachable_node_gid)) {
          PANDO_CHECK(reachable_set.pushBack(reachable_node_gid));
          PANDO_CHECK(frontier.emplace(reachable_node_gid));
        }
      }
    }
  }
  PANDO_CHECK(state.rrr_sets.pushBack(reachable_set));
  frontier.deinitialize();
}

void wf4::internal::FillNodeValues(wf4::NetworkGraph& graph,
                                   const wf4::NetworkGraph::VertexTopologyID& node) {
  NetworkNode node_data = graph.getData(node);
  for (auto edge_handle : graph.edges(node)) {
    wf4::NetworkEdge edge = graph.getEdgeData(edge_handle);

    if (edge.type == agile::TYPES::SALE) {
      *node_data.sold_ += edge.amount_;
    } else if (edge.type == agile::TYPES::PURCHASE) {
      *node_data.bought_ += edge.amount_;
      node_data.desired_ += edge.amount_;
    }
  }
  graph.setData(node, node_data);
}

void wf4::internal::CalculateEdgeProbability(EdgeProbabilityState& state,
                                             const wf4::NetworkGraph::VertexTopologyID& node) {
  NetworkNode node_data = state.graph.getData(node);
  double amount_sold = *node_data.sold_;
  for (auto edge_handle : state.graph.edges(node)) {
    NetworkEdge edge = state.graph.getEdgeData(edge_handle);

    state.total_sales.add(edge.amount_);
    if (edge.type == agile::TYPES::SALE && amount_sold > 0) {
      edge.weight_ = edge.amount_ / amount_sold;
      state.graph.setEdgeData(edge_handle, edge);
      state.total_edge_weights.add(edge.weight_);
    } else if (edge.type == agile::TYPES::PURCHASE) {
      NetworkNode dst_data = state.graph.getData(state.graph.getEdgeDst(edge_handle));
      if (*dst_data.sold_ > 0) {
        edge.weight_ = edge.amount_ / *dst_data.sold_;
        state.graph.setEdgeData(edge_handle, edge);
        state.total_edge_weights.add(edge.weight_);
      }
    }
  }
}
