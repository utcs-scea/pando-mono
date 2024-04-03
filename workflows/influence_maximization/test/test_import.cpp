// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/global_ptr.hpp>

#include <pando-wf4-galois/import.hpp>

namespace {

const char* someFile = "some_file.csv";

void checkParsedEdge(galois::ParsedEdges<wf4::FullNetworkEdge> result,
                     wf4::FullNetworkEdge expected, agile::TYPES expected_inverse,
                     uint64_t expected_num_edges) {
  EXPECT_EQ(result.isEdge, true);
  EXPECT_EQ(result.has2Edges, expected_num_edges == 2);

  wf4::FullNetworkEdge edge0 = result.edge1;

  EXPECT_EQ(edge0.src, expected.src);
  EXPECT_EQ(edge0.dst, expected.dst);
  EXPECT_EQ(edge0.type, expected.type);
  EXPECT_EQ(edge0.srcType, expected.srcType);
  EXPECT_EQ(edge0.dstType, expected.dstType);
  EXPECT_FLOAT_EQ(edge0.amount_, expected.amount_);
  EXPECT_EQ(edge0.topic, expected.topic);

  if (result.has2Edges) {
    wf4::FullNetworkEdge edge1 = result.edge2;
    EXPECT_EQ(edge1.type, expected_inverse);
    EXPECT_NE(edge0.type, edge1.type);
    EXPECT_EQ(edge0.src, edge1.dst);
    EXPECT_EQ(edge0.dst, edge1.src);
    EXPECT_EQ(edge0.srcType, edge1.dstType);
    EXPECT_EQ(edge0.dstType, edge1.srcType);
    EXPECT_FLOAT_EQ(edge0.amount_, edge1.amount_);
    EXPECT_EQ(edge0.topic, edge1.topic);
  }
}

const uint64_t num_nodes = 19;
const uint64_t num_projected_nodes = 17;
const uint64_t num_projected_edges = (num_projected_nodes) * (num_projected_nodes - 1);
const uint64_t num_edges = num_projected_edges + 4 * num_projected_nodes + 1;
const uint64_t projected_node_offset = num_projected_nodes;
const uint64_t projected_edge_offset = num_projected_edges + 4 * num_projected_nodes;

wf4::FullNetworkGraph generateTestFullGraph() {
  wf4::FullNetworkGraph graph{};
  pando::Vector<wf4::FullNetworkNode> vertices;
  pando::Vector<galois::GenericEdge<wf4::FullNetworkEdge>> edges;
  EXPECT_EQ(vertices.initialize(num_nodes), pando::Status::Success);
  EXPECT_EQ(edges.initialize(num_edges), pando::Status::Success);

  // Data that will be projected out
  {
    wf4::FullNetworkNode node(projected_node_offset + 0, agile::TYPES::DEVICE);
    vertices[projected_node_offset + 0] = node;
    wf4::FullNetworkNode node2(projected_node_offset + 1, agile::TYPES::PERSON);
    vertices[projected_node_offset + 1] = node2;

    edges[projected_edge_offset] = galois::GenericEdge(
        projected_node_offset + 1, projected_node_offset + 0,
        wf4::FullNetworkEdge(projected_node_offset + 1, projected_node_offset + 0,
                             agile::TYPES::SALE, agile::TYPES::PERSON, agile::TYPES::DEVICE, 1,
                             8486));
  }

  for (uint64_t i = 0; i < num_projected_nodes; i++) {
    wf4::FullNetworkNode node(i, agile::TYPES::PERSON);
    node.sold_ = i * i;
    node.bought_ = (num_projected_nodes + i) * (num_projected_nodes - (i + 1)) / 2;
    vertices[i] = node;
  }
  // Every node sells to every node with a global ID less than itself
  // And so every node buys from every node with a global ID more than itself
  // We set the edge weight to be equal to the global ID of the seller
  // Vertex 0 does not sell anything
  uint64_t edge_count = 0;
  for (uint64_t src = 0; src < num_projected_nodes; src++) {
    // will be projected out
    edges[edge_count++] =
        galois::GenericEdge(src, 1,
                            wf4::FullNetworkEdge(src, 2, agile::TYPES::AUTHOR, agile::TYPES::PERSON,
                                                 agile::TYPES::PERSON, 1, 8486));
    // will be projected out
    edges[edge_count++] =
        galois::GenericEdge(src, 1,
                            wf4::FullNetworkEdge(src, 0, agile::TYPES::AUTHOR, agile::TYPES::PERSON,
                                                 agile::TYPES::DEVICE, 1, 8486));
    for (uint64_t dst = 0; dst < src; dst++) {
      edges[edge_count++] = galois::GenericEdge(
          src, dst,
          wf4::FullNetworkEdge(src, dst, agile::TYPES::SALE, agile::TYPES::PERSON,
                               agile::TYPES::PERSON, src, 8486));
    }
    // will be projected out
    edges[edge_count++] =
        galois::GenericEdge(src, 1,
                            wf4::FullNetworkEdge(src, 1, agile::TYPES::SALE, agile::TYPES::PERSON,
                                                 agile::TYPES::PERSON, 1, 8487));
    for (uint64_t dst = src + 1; dst < num_projected_nodes; dst++) {
      edges[edge_count++] = galois::GenericEdge(
          src, dst,
          wf4::FullNetworkEdge(src, dst, agile::TYPES::PURCHASE, agile::TYPES::PERSON,
                               agile::TYPES::PERSON, dst, 8486));
    }
    // will be projected out
    edges[edge_count++] =
        galois::GenericEdge(src, 3,
                            wf4::FullNetworkEdge(src, 3, agile::TYPES::SALE, agile::TYPES::PERSON,
                                                 agile::TYPES::PERSON, 0, 8486));
  }

  EXPECT_EQ(graph.initialize(vertices, edges), pando::Status::Success);
  vertices.deinitialize();
  edges.deinitialize();
  return graph;
}

wf4::NetworkGraph generateTestGraph() {
  wf4::NetworkGraph graph{};
  pando::Vector<wf4::NetworkNode> vertices;
  pando::Vector<galois::GenericEdge<wf4::NetworkEdge>> edges;
  EXPECT_EQ(vertices.initialize(num_projected_nodes), pando::Status::Success);
  EXPECT_EQ(edges.initialize(num_projected_edges), pando::Status::Success);

  for (uint64_t i = 0; i < num_projected_nodes; i++) {
    wf4::NetworkNode node;
    EXPECT_EQ(node.initialize(i), pando::Status::Success);
    *node.sold_ = i * i;
    *node.bought_ = (num_projected_nodes + i) * (num_projected_nodes - (i + 1)) / 2;
    vertices[i] = node;
  }
  // Every node sells to every node with a global ID less than itself
  // And so every node buys from every node with a global ID more than itself
  // We set the edge weight to be equal to the global ID of the seller
  // Vertex 0 does not sell anything
  uint64_t edge_count = 0;
  for (uint64_t src = 0; src < num_projected_nodes; src++) {
    for (uint64_t dst = 0; dst < src; dst++) {
      edges[edge_count++] =
          galois::GenericEdge(src, dst, wf4::NetworkEdge(src, agile::TYPES::SALE));
    }
    for (uint64_t dst = src + 1; dst < num_projected_nodes; dst++) {
      edges[edge_count++] =
          galois::GenericEdge(src, dst, wf4::NetworkEdge(dst, agile::TYPES::PURCHASE));
    }
  }

  EXPECT_EQ(graph.initialize(vertices, edges), pando::Status::Success);
  vertices.deinitialize();
  edges.deinitialize();
  return graph;
}

void graphsEqual(wf4::NetworkGraph actual, wf4::NetworkGraph expected) {
  EXPECT_EQ(actual.size(), expected.size());
  for (uint64_t i = 0; i < num_projected_nodes; i++) {
    wf4::NetworkNode actual_node = actual.getData(actual.getTopologyID(i));
    bool found = false;
    for (uint64_t j = 0; j < num_projected_nodes; j++) {
      wf4::NetworkNode expected_node = expected.getData(expected.getTopologyID(j));
      if (actual_node.id == expected_node.id) {
        found = true;
        EXPECT_FLOAT_EQ(*actual_node.bought_, *expected_node.bought_);
        EXPECT_FLOAT_EQ(*actual_node.sold_, *expected_node.sold_);
        EXPECT_FLOAT_EQ(actual_node.desired_, expected_node.desired_);
        std::printf("node topology id: %lu, node token id: %lu\n", i, actual_node.id);
        EXPECT_EQ(actual.getNumEdges(actual.getTopologyID(i)),
                  expected.getNumEdges(expected.getTopologyID(j)));
        for (uint64_t e = 0; e < actual.getNumEdges(actual.getTopologyID(i)); e++) {
          wf4::NetworkEdge actual_edge = actual.getEdgeData(actual.getTopologyID(i), e);
          wf4::NetworkEdge expected_edge = expected.getEdgeData(expected.getTopologyID(j), e);
          EXPECT_FLOAT_EQ(actual_edge.amount_, expected_edge.amount_);
          EXPECT_FLOAT_EQ(actual_edge.weight_, expected_edge.weight_);
          EXPECT_EQ(actual_edge.type, expected_edge.type);
        }
      }
    }
    EXPECT_EQ(found, true);
  }
}

} // namespace

TEST(Import, Parse) {
  galois::EdgeParser<wf4::FullNetworkEdge> cyber_parser(galois::StringView(someFile).toArray(),
                                                        &wf4::ParseCyberLine);
  galois::EdgeParser<wf4::FullNetworkEdge> social_parser(galois::StringView(someFile).toArray(),
                                                         &wf4::ParseSocialLine);
  galois::EdgeParser<wf4::FullNetworkEdge> uses_parser(galois::StringView(someFile).toArray(),
                                                       &wf4::ParseUsesLine);
  galois::EdgeParser<wf4::FullNetworkEdge> commercial_parser(galois::StringView(someFile).toArray(),
                                                             &wf4::ParseCommercialLine);
  uint64_t half_max = std::numeric_limits<uint64_t>::max() / 2;

  const char* invalid = "invalid,,,1615340315424362057,1116314936447312244,,,2/11/2018,,";

  const char* sale = "Sale,1552474,1928788,,8/21/2018,,,";
  const char* weighted_sale = "Sale,299156,458364,8486,,,,3.0366367403882406";
  const char* communication = "0,217661,172800,0,6,26890,94857,6,5,1379,1770";
  const char* friend_edge = "5,679697";
  const char* uses = "12,311784";

  galois::ParsedEdges<wf4::FullNetworkEdge> result;
  result = commercial_parser.parser(invalid);
  EXPECT_EQ(result.isEdge, false);
  EXPECT_EQ(result.has2Edges, false);

  result = commercial_parser.parser(sale);
  checkParsedEdge(result,
                  wf4::FullNetworkEdge(1552474UL, 1928788UL, agile::TYPES::SALE,
                                       agile::TYPES::PERSON, agile::TYPES::PERSON, 0, 0),
                  agile::TYPES::PURCHASE, 2);
  result = commercial_parser.parser(weighted_sale);
  checkParsedEdge(result,
                  wf4::FullNetworkEdge(299156UL, 458364UL, agile::TYPES::SALE, agile::TYPES::PERSON,
                                       agile::TYPES::PERSON, 3.0366367403882406, 8486),
                  agile::TYPES::PURCHASE, 2);
  result = cyber_parser.parser(communication);
  checkParsedEdge(
      result,
      wf4::FullNetworkEdge(half_max + 0UL, half_max + 217661UL, agile::TYPES::COMMUNICATION,
                           agile::TYPES::DEVICE, agile::TYPES::DEVICE, 0, 0),
      agile::TYPES::NONE, 2);
  result = social_parser.parser(friend_edge);
  checkParsedEdge(result,
                  wf4::FullNetworkEdge(5UL, 679697UL, agile::TYPES::FRIEND, agile::TYPES::PERSON,
                                       agile::TYPES::PERSON, 0, 0),
                  agile::TYPES::NONE, 2);
  result = uses_parser.parser(uses);
  checkParsedEdge(result,
                  wf4::FullNetworkEdge(12UL, half_max + 311784UL, agile::TYPES::USES,
                                       agile::TYPES::PERSON, agile::TYPES::DEVICE, 0, 0),
                  agile::TYPES::NONE, 2);
}

TEST(Import, GeneratedGraph) {
  wf4::NetworkGraph test_graph = generateTestGraph();
  uint64_t vertex_count = 0;
  uint64_t edge_count = 0;
  for (wf4::NetworkGraph::VertexTopologyID node : test_graph.vertices()) {
    vertex_count++;
    wf4::NetworkNode node_data = test_graph.getData(node);
    double bought = *node_data.bought_;
    EXPECT_EQ(test_graph.getTokenID(node), node_data.id);
    EXPECT_EQ(bought, (num_projected_nodes + node_data.id) *
                          (num_projected_nodes - (node_data.id + 1)) / 2);
    for (wf4::NetworkGraph::EdgeHandle edge : test_graph.edges(node)) {
      edge_count++;
      wf4::NetworkEdge edge_data = test_graph.getEdgeData(edge);
      uint64_t dst_id = test_graph.getTokenID(test_graph.getEdgeDst(edge));
      EXPECT_NE(dst_id, node_data.id);
      EXPECT_EQ(dst_id < node_data.id, edge_data.type == agile::TYPES::SALE);
      EXPECT_EQ(dst_id > node_data.id, edge_data.type == agile::TYPES::PURCHASE);
      if (edge_data.type == agile::TYPES::SALE) {
        EXPECT_EQ(edge_data.amount_, node_data.id);
      } else {
        EXPECT_EQ(edge_data.amount_, dst_id);
      }
    }
  }
  EXPECT_EQ(vertex_count, num_projected_nodes);
  EXPECT_EQ(edge_count, num_projected_edges);
}

TEST(Import, Projection) {
  wf4::FullNetworkGraph full_graph = generateTestFullGraph();
  wf4::NetworkGraph projected_graph = wf4::ProjectGraph(full_graph);
  wf4::NetworkGraph expected_graph = generateTestGraph();
  graphsEqual(projected_graph, expected_graph);
  expected_graph.deinitialize();
  projected_graph.deinitialize();
}
