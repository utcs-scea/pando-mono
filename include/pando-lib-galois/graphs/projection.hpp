// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_PROJECTION_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_PROJECTION_HPP_

#include <pando-rt/export.h>

#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

namespace galois::internal {

template <typename Graph, typename Projection, typename V, typename E>
struct ProjectionState {
  ProjectionState() = default;
  ProjectionState(Graph oldGraph_, Projection projection_,
                  galois::PerThreadVector<V> projectedVertices_,
                  galois::PerThreadVector<E> projectedEdges_,
                  galois::PerThreadVector<uint64_t> projectedEdgeDestinations_,
                  galois::PerThreadVector<uint64_t> projectedEdgeCounts_)
      : oldGraph(oldGraph_),
        projection(projection_),
        projectedVertices(projectedVertices_),
        projectedEdges(projectedEdges_),
        projectedEdgeDestinations(projectedEdgeDestinations_),
        projectedEdgeCounts(projectedEdgeCounts_) {}

  Graph oldGraph;
  Projection projection;
  galois::PerThreadVector<V> projectedVertices;
  galois::PerThreadVector<E> projectedEdges;
  galois::PerThreadVector<uint64_t> projectedEdgeDestinations;
  galois::PerThreadVector<uint64_t> projectedEdgeCounts;
};

} // namespace galois::internal

namespace galois {

/**
 * @brief Project a graph given some Projection class
 * @warning This consumes the original graph
 * @note Tests for Project exist in
 * https://github.com/AMDResearch/PANDO-wf4-gal-root/blob/main/test/test_import.cpp
 */
template <class OldGraph, class NewGraph, class Projection>
NewGraph Project(OldGraph&& oldGraph, Projection projection) {
  using OldVertexType = typename OldGraph::VertexData;
  using OldEdgeType = typename OldGraph::EdgeData;
  using OldVertexTopologyID = typename OldGraph::VertexTopologyID;
  using NewVertexType = typename NewGraph::VertexData;
  using NewEdgeType = typename NewGraph::EdgeData;

  galois::PerThreadVector<NewVertexType> projectedVertices;
  galois::PerThreadVector<NewEdgeType> projectedEdges;
  galois::PerThreadVector<uint64_t> projectedEdgeDestinations;
  galois::PerThreadVector<uint64_t> projectedEdgeCounts;
  PANDO_CHECK(projectedVertices.initialize());
  PANDO_CHECK(projectedEdges.initialize());
  PANDO_CHECK(projectedEdgeDestinations.initialize());
  PANDO_CHECK(projectedEdgeCounts.initialize());
  internal::ProjectionState state(oldGraph, projection, projectedVertices, projectedEdges,
                                  projectedEdgeDestinations, projectedEdgeCounts);

  galois::doAll(
      state, oldGraph.vertices(),
      +[](internal::ProjectionState<OldGraph, Projection, NewVertexType, NewEdgeType>& state,
          OldVertexTopologyID node) {
        if (!state.projection.KeepNode(state.oldGraph, node)) {
          return;
        }
        uint64_t keptEdges = 0;
        for (auto edge : state.oldGraph.edges(node)) {
          OldEdgeType edgeData = state.oldGraph.getEdgeData(edge);
          auto dstNode = state.oldGraph.getEdgeDst(edge);
          if (!state.projection.KeepEdge(state.oldGraph, edgeData, node, dstNode)) {
            continue;
          }
          keptEdges++;
          PANDO_CHECK(state.projectedEdges.pushBack(
              state.projection.ProjectEdge(state.oldGraph, edgeData, node, dstNode)));
          PANDO_CHECK(state.projectedEdgeDestinations.pushBack(edgeData.dst));
        }
        if (state.projection.KeepEdgeLessMasters() || keptEdges > 0) {
          OldVertexType nodeData = state.oldGraph.getData(node);
          PANDO_CHECK(state.projectedVertices.pushBack(
              state.projection.ProjectNode(state.oldGraph, nodeData, node)));
          PANDO_CHECK(state.projectedEdgeCounts.pushBack(keptEdges));
        }
      });

  // edge sources are sorted by construction due to no pre-emption
  NewGraph newGraph;
  PANDO_CHECK(newGraph.initialize(oldGraph, projectedVertices, projectedEdges,
                                  projectedEdgeDestinations, projectedEdgeCounts));
  std::printf("Projected vertices: %lu\n", newGraph.size());
  std::printf("Projected edges: %lu\n", newGraph.sizeEdges());
  oldGraph.deinitialize();
  projectedVertices.deinitialize();
  projectedEdges.deinitialize();
  projectedEdgeDestinations.deinitialize();
  projectedEdgeCounts.deinitialize();
  return newGraph;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_PROJECTION_HPP_
