// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GRAPHS_MHR_GRAPH_HPP_
#define PANDO_WF1_GRAPHS_MHR_GRAPH_HPP_

#include <pando-rt/export.h>

#include <charconv>

#include <pando-rt/containers/vector.hpp>

#include <pando-lib-galois/graphs/dist_local_csr.hpp>

namespace wf1 {

class MHRNode;
class MHREdge;
typedef galois::DistLocalCSR<MHRNode, MHREdge> MHRGraph;
class RelationFeatures;

using NodeFiles = pando::Vector<galois::VertexParser<wf1::MHRNode>>;
using EdgeFiles = pando::Vector<galois::EdgeParser<wf1::MHREdge>>;

const uint64_t EMBEDDING_FEATURE_SIZE = 450;

enum class MHR_ENTITY { PERSON, UNIVERSITY, NONE };

const uint64_t AWARD_WINNER_TYPE = 207;
const uint64_t WORKS_IN_TYPE = 3;
const uint64_t AFFILIATED_WITH_TYPE = 40;

MHRNode ParseEmbeddingLine(const char* line);
galois::ParsedEdges<MHREdge> ParseRelationLine(const char* line);

class MHRNode {
public:
  MHRNode() = default;
  explicit MHRNode(pando::Vector<galois::StringView>&& tokens) {
    galois::StringView token0 = tokens[0];
    id = token0.getU64();

    PANDO_CHECK(features.initialize(tokens.size() - 1));
    for (uint64_t i = 1; i < tokens.size(); i++) {
      galois::StringView token = tokens[i];
      double parsedValue;
      std::from_chars(token.get(), token.get() + token.size(), parsedValue);
      features[i - 1] = parsedValue;
    }
    tokens.deinitialize();
  }
  MHRNode(uint64_t, agile::TYPES) {
    PANDO_ABORT("This should never be called.");
  }

  void deinitialize() {
    features.deinitialize();
  }

  uint64_t id = 0;
  MHR_ENTITY type = MHR_ENTITY::NONE;
  pando::Vector<double> features{};
};

class MHREdge {
public:
  MHREdge() = default;
  explicit MHREdge(pando::Vector<galois::StringView>&& tokens) {
    galois::StringView token1 = tokens[1];
    galois::StringView token2 = tokens[2];
    galois::StringView token3 = tokens[3];
    src = token1.getU64();
    type = token2.getU64();
    dst = token3.getU64();
    tokens.deinitialize();
  }

  uint64_t src = 0;
  uint64_t dst = 0;
  uint64_t type = 0;
  bool mirror = false;
};

class RelationFeatures {
public:
  RelationFeatures() = default;

  [[nodiscard]] pando::Status initialize(galois::VertexParser<wf1::MHRNode>&& parser);
  void deinitialize();

  pando::Vector<double> getRelationFeature(wf1::MHRGraph::VertexTokenID relationID);

private:
  [[nodiscard]] pando::Status importFeatures(galois::VertexParser<wf1::MHRNode>& parser);

  galois::HostLocalStorage<galois::HashTable<MHRGraph::VertexTokenID, pando::Vector<double>>>
      features;
};

namespace internal {

template <typename Graph>
struct MHRGraphProjection {
  using VertexTopologyID = typename Graph::VertexTopologyID;

  bool KeepEdgeLessMasters() const {
    return false;
  }

  bool KeepNode(Graph& graph, VertexTopologyID node) {
    // Nodes are filtered by whether or not they have any kept edges
    // Remaining node types should be: Person, University, Work Field, Award
    return true;
  }
  bool KeepEdge(Graph&, const wf1::MHREdge& edge, VertexTopologyID, VertexTopologyID) {
    return edge.type == AWARD_WINNER_TYPE || edge.type == WORKS_IN_TYPE ||
           edge.type == AFFILIATED_WITH_TYPE;
  }

  wf1::MHRNode ProjectNode(Graph& graph, wf1::MHRNode& nodeData, VertexTopologyID node) {
    // TODO(Patrick) consider fixing feature locality here
    for (wf1::MHRGraph::EdgeHandle edge : graph.edges(node)) {
      wf1::MHREdge edgeData = graph.getEdgeData(edge);
      if (!edgeData.mirror && edgeData.type == wf1::AWARD_WINNER_TYPE) {
        nodeData.type = wf1::MHR_ENTITY::PERSON;
        break;
      } else if (edgeData.mirror && edgeData.type == wf1::AFFILIATED_WITH_TYPE) {
        if (nodeData.id == 22174494 /*generic affiliation*/
            || nodeData.id == 78111271 || nodeData.id == 51562303 ||
            nodeData.id == 344618                               /*self-employment*/
            || nodeData.id == 4209802 || nodeData.id == 9294723 /*generic description*/
            || nodeData.id == 35693055 /*another generic entity*/) {
          break;
        }
        nodeData.type = wf1::MHR_ENTITY::UNIVERSITY;
        break;
      }
    }
    pando::Vector<double> localFeatures{};
    PANDO_CHECK(localFeatures.initialize(nodeData.features.size()));
    for (uint64_t i = 0; i < localFeatures.size(); i++) {
      localFeatures[i] = nodeData.features[i];
    }
    nodeData.features.deinitialize();
    nodeData.features = localFeatures;
    return nodeData;
  }
  wf1::MHREdge ProjectEdge(Graph&, const wf1::MHREdge& edge, VertexTopologyID, VertexTopologyID) {
    return edge;
  }
};

} // namespace internal

} // namespace wf1

#endif // PANDO_WF1_GRAPHS_MHR_GRAPH_HPP_
