// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF4_GALOIS_INFLUENCE_MAXIMIZATION_HPP_
#define PANDO_WF4_GALOIS_INFLUENCE_MAXIMIZATION_HPP_

#include <pando-rt/export.h>

#include "graph.hpp"

#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/containers/vector.hpp>

namespace wf4 {

typedef galois::PerThreadVector<pando::Vector<NetworkGraph::VertexTokenID>> ReverseReachableSet;

void CalculateEdgeProbabilities(NetworkGraph& graph);
ReverseReachableSet GetRandomReverseReachableSets(NetworkGraph& graph, uint64_t num_sets,
                                                  uint64_t seed);
pando::Vector<NetworkGraph::VertexTokenID> GetInfluentialNodes(
    wf4::NetworkGraph& graph, ReverseReachableSet&& reachability_sets, uint64_t num_nodes);

namespace internal {

struct EdgeProbabilityState {
  EdgeProbabilityState() = default;
  EdgeProbabilityState(wf4::NetworkGraph graph_, galois::DAccumulator<double> total_edge_weights_,
                       galois::DAccumulator<double> total_sales_)
      : graph(graph_), total_edge_weights(total_edge_weights_), total_sales(total_sales_) {}

  wf4::NetworkGraph graph;
  galois::DAccumulator<double> total_edge_weights;
  galois::DAccumulator<double> total_sales;
};

struct RRRState {
  RRRState() = default;
  RRRState(wf4::NetworkGraph graph_, wf4::ReverseReachableSet rrr_sets_, uint64_t seed_)
      : graph(graph_), rrr_sets(rrr_sets_), seed(seed_) {}

  wf4::NetworkGraph graph;
  wf4::ReverseReachableSet rrr_sets;
  uint64_t seed;
} typedef RRRState;

struct InfluentialState {
  InfluentialState() = default;
  InfluentialState(wf4::NetworkGraph graph_, wf4::NetworkGraph::VertexTokenID influential_node_,
                   galois::WaitGroup::HandleType wgh_)
      : graph(graph_), influential_node(influential_node_), wgh(wgh_) {}

  wf4::NetworkGraph graph;
  wf4::NetworkGraph::VertexTokenID influential_node;
  galois::WaitGroup::HandleType wgh;
} typedef InfluentialState;

struct LocalMaxNode {
  LocalMaxNode() : max_node(0), max_influence(0) {}
  LocalMaxNode(wf4::NetworkGraph::VertexTokenID node, uint64_t influence)
      : max_node(node), max_influence(influence) {}

  wf4::NetworkGraph::VertexTokenID max_node;
  uint64_t max_influence;
} typedef LocalMaxNode;

struct MaxState {
  MaxState() = default;
  MaxState(wf4::NetworkGraph graph_,
           galois::PerThreadVector<wf4::internal::LocalMaxNode> max_array_,
           galois::DAccumulator<uint64_t> total_influence_)
      : graph(graph_), max_array(max_array_), total_influence(total_influence_) {}

  wf4::NetworkGraph graph;
  galois::PerThreadVector<wf4::internal::LocalMaxNode> max_array;
  galois::DAccumulator<uint64_t> total_influence;
} typedef MaxState;

void FillNodeValues(wf4::NetworkGraph& graph, const wf4::NetworkGraph::VertexTopologyID& node);
void CalculateEdgeProbability(EdgeProbabilityState& state,
                              const wf4::NetworkGraph::VertexTopologyID& node);

void GenerateRandomReversibleReachableSet(RRRState& state, uint64_t set_id, uint64_t);

void RemoveReachableSetWithInfluentialNode(
    InfluentialState& state,
    pando::GlobalRef<pando::Vector<wf4::NetworkGraph::VertexTokenID>> reachability_set);
void RemoveReachableSetsWithInfluentialNode(wf4::NetworkGraph& graph,
                                            wf4::ReverseReachableSet& reachability_sets,
                                            wf4::NetworkGraph::VertexTokenID influential_node);

void FindLocalMaxNode(MaxState& state, wf4::NetworkGraph::VertexTopologyID node);
wf4::NetworkGraph::VertexTokenID GetMostInfluentialNode(wf4::NetworkGraph& graph, uint64_t rank);

} // namespace internal

} // namespace wf4

#endif // PANDO_WF4_GALOIS_INFLUENCE_MAXIMIZATION_HPP_
