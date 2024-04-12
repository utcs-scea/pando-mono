// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF4_IMPORT_HPP_
#define PANDO_WF4_IMPORT_HPP_

#include <limits>
#include <utility>

#include "graph.hpp"

namespace wf4 {

using InputFiles = pando::Vector<galois::EdgeParser<wf4::FullNetworkEdge>>;
FullNetworkGraph ImportData(InputFiles&& input_files);
NetworkGraph ProjectGraph(FullNetworkGraph full_graph);

galois::ParsedEdges<wf4::FullNetworkEdge> ParseCommercialLine(const char* line);
galois::ParsedEdges<wf4::FullNetworkEdge> ParseCyberLine(const char* line);
galois::ParsedEdges<wf4::FullNetworkEdge> ParseSocialLine(const char* line);
galois::ParsedEdges<wf4::FullNetworkEdge> ParseUsesLine(const char* line);

namespace internal {

galois::DistArray<wf4::FullNetworkEdge> ImportFiles(wf4::InputFiles input_files);

template <typename Graph>
struct NetworkGraphProjection {
  using VertexTopologyID = typename Graph::VertexTopologyID;

  bool KeepEdgeLessMasters() const {
    return false;
  }

  bool KeepNode(Graph& graph, VertexTopologyID node) {
    wf4::FullNetworkNode node_data = graph.getData(node);
    return node_data.type == agile::TYPES::PERSON;
  }
  bool KeepEdge(Graph&, const wf4::FullNetworkEdge& edge, VertexTopologyID, VertexTopologyID) {
    return (edge.type == agile::TYPES::PURCHASE || edge.type == agile::TYPES::SALE) &&
           edge.topic == 8486 && edge.amount_ > 0 && edge.dstType == agile::TYPES::PERSON;
  }

  wf4::NetworkNode ProjectNode(Graph&, const wf4::FullNetworkNode& node, VertexTopologyID) {
    wf4::NetworkNode projected_node;
    PANDO_CHECK(projected_node.initialize(node));
    return projected_node;
  }
  wf4::NetworkEdge ProjectEdge(Graph&, const wf4::FullNetworkEdge& edge, VertexTopologyID,
                               VertexTopologyID) {
    return wf4::NetworkEdge(edge);
  }
};

} // namespace internal

} // namespace wf4

#endif // PANDO_WF4_IMPORT_HPP_
