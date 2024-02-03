// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <numeric>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-rt/memory/memory_guard.hpp>

using Graph = galois::DistLocalCSR<galois::WMDVertex, galois::WMDEdge>;

void getVerticesAndEdges(
    std::string& filename, std::unordered_map<std::uint64_t, galois::WMDVertex>& vertices,
    std::unordered_map<std::uint64_t, std::vector<galois::WMDEdge>>& hashTable) {
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("//") != std::string::npos || line.find("#") != std::string::npos) {
      continue;
    } else if (line.find("/*") != std::string::npos || line.find("*/") != std::string::npos) {
      continue;
    } else {
      const char* ptr = line.c_str();
      pando::Vector<galois::StringView> tokens = galois::splitLine(ptr, ',', 10);
      bool isNode = tokens[0] == galois::StringView("Person") ||
                    tokens[0] == galois::StringView("ForumEvent") ||
                    tokens[0] == galois::StringView("Forum") ||
                    tokens[0] == galois::StringView("Publication") ||
                    tokens[0] == galois::StringView("Topic");
      if (isNode) {
        galois::WMDVertex vertex(tokens);
        vertices[vertex.id] = vertex;
        // check if the vertex is already in the hash table
        if (hashTable.find(vertex.id) == hashTable.end()) {
          hashTable[vertex.id] = std::vector<galois::WMDEdge>();
        }
      } else {
        galois::WMDEdge edge(tokens);
        if (hashTable.find(edge.src) == hashTable.end()) {
          hashTable[edge.src] = std::vector<galois::WMDEdge>();
        }
        hashTable[edge.src].push_back(edge);
        // Inverse edge
        agile::TYPES inverseEdgeType = agile::TYPES::NONE;
        if (tokens[0] == galois::StringView("Sale")) {
          inverseEdgeType = agile::TYPES::PURCHASE;
        } else if (tokens[0] == galois::StringView("Author")) {
          inverseEdgeType = agile::TYPES::WRITTENBY;
        } else if (tokens[0] == galois::StringView("Includes")) {
          inverseEdgeType = agile::TYPES::INCLUDEDIN;
        } else if (tokens[0] == galois::StringView("HasTopic")) {
          inverseEdgeType = agile::TYPES::TOPICIN;
        } else if (tokens[0] == galois::StringView("HasOrg")) {
          inverseEdgeType = agile::TYPES::ORG_IN;
        } else {
          ASSERT_TRUE(false) << "Should never be reached" << std::endl;
        }
        galois::WMDEdge inverseEdge(edge.dst, edge.src, inverseEdgeType, edge.dstType,
                                    edge.srcType);
        if (hashTable.find(inverseEdge.src) == hashTable.end()) {
          hashTable[inverseEdge.src] = std::vector<galois::WMDEdge>();
        }
        hashTable[inverseEdge.src].push_back(inverseEdge);
      }
      tokens.deinitialize();
    }
  }
}

class DLCSRInit : public ::testing::TestWithParam<const char*> {};

TEST_P(DLCSRInit, initializeWMD) {
  using ET = galois::WMDEdge;
  using VT = galois::WMDVertex;
  Graph graph;
  pando::Array<char> filename;
  std::string wmdFile = GetParam();
  bool isEdgelist = false;
  PANDO_CHECK(filename.initialize(wmdFile.size()));
  for (uint64_t i = 0; i < wmdFile.size(); i++)
    filename[i] = wmdFile[i];
  pando::GlobalPtr<Graph> dGraphPtr = static_cast<decltype(dGraphPtr)>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(Graph)));
  PANDO_CHECK(fmap(*dGraphPtr, initializeWMD, filename, isEdgelist));

  // Validate
  std::unordered_map<std::uint64_t, std::vector<ET>> goldenTable;
  std::unordered_map<std::uint64_t, VT> goldenVertices;
  getVerticesAndEdges(wmdFile, goldenVertices, goldenTable);

  EXPECT_EQ(goldenVertices.size(), lift(*dGraphPtr, size));
  // Iterate over vertices
  std::uint64_t vid = 0;

  typename Graph::VertexRange vertices = lift(*dGraphPtr, vertices);
  for (typename Graph::VertexTopologyID vert : vertices) {
    EXPECT_EQ(vid, fmap(*dGraphPtr, getVertexIndex, vert));
    vid++;
    typename Graph::VertexTokenID id = fmap(*dGraphPtr, getTokenID, vert);

    typename Graph::VertexData vertex = fmap(*dGraphPtr, getData, vert);
    EXPECT_NE(goldenVertices.find(id), goldenVertices.end())
        << "Failed to get tok_id:" << id << "\t with index: " << (vid - 1);

    VT goldenVertex = goldenVertices[id];
    vertex = fmap(*dGraphPtr, getData, vert);
    EXPECT_EQ(goldenVertex.id, vertex.id);
    goldenVertex.id = 0;
    EXPECT_EQ(goldenVertex.type, vertex.type);
    goldenVertex.type = agile::TYPES::NONE;
    EXPECT_EQ(goldenVertex.edges, vertex.edges);
    goldenVertex.edges = 0;
    fmapVoid(*dGraphPtr, setData, vert, goldenVertex);
    vertex = fmap(*dGraphPtr, getData, vert);
    EXPECT_EQ(0, vertex.id);
    EXPECT_EQ(agile::TYPES::NONE, vertex.type);
    EXPECT_EQ(0, vertex.edges);

    // Iterate over edges
    EXPECT_NE(goldenTable.find(id), goldenTable.end())
        << "Failed to find edges with tok_id:" << id << "\t with index: " << (vid - 1);
    std::vector<ET> goldenEdges = goldenTable[id];
    EXPECT_EQ(goldenEdges.size(), fmap(*dGraphPtr, getNumEdges, vert))
        << "Number of edges for tok_id: " << id << "\t with index: " << (vid - 1);
    for (typename Graph::EdgeHandle eh : fmap(*dGraphPtr, edges, vert)) {
      typename Graph::EdgeData eData = fmap(*dGraphPtr, getEdgeData, eh);

      typename Graph::VertexTokenID dstTok =
          fmap(*dGraphPtr, getTokenID, fmap(*dGraphPtr, getEdgeDst, eh));
      EXPECT_EQ(eData.dst, dstTok);

      auto goldenEdgeIt = std::find(goldenEdges.begin(), goldenEdges.end(), eData);
      EXPECT_NE(goldenEdgeIt, goldenEdges.end())
          << "Unable to find edge with src_tok: " << id << "\tand dst_tok: " << dstTok
          << "\tat vertex: " << (vid - 1);

      ET goldenEdge = *goldenEdgeIt;
      goldenEdge.src = 0;
      goldenEdge.dst = 0;
      goldenEdge.type = agile::TYPES::NONE;
      goldenEdge.srcType = agile::TYPES::NONE;
      goldenEdge.dstType = agile::TYPES::NONE;
      fmapVoid(*dGraphPtr, setEdgeData, eh, goldenEdge);
      eData = fmap(*dGraphPtr, getEdgeData, eh);
      EXPECT_EQ(0, eData.src);
      EXPECT_EQ(0, eData.dst);
      EXPECT_EQ(agile::TYPES::NONE, eData.type);
      EXPECT_EQ(agile::TYPES::NONE, eData.srcType);
      EXPECT_EQ(agile::TYPES::NONE, eData.dstType);
    }
  }
  // deinitialize
  liftVoid(*dGraphPtr, deinitialize);
}

INSTANTIATE_TEST_SUITE_P(SmallFiles, DLCSRInit,
                         ::testing::Values("/pando/graphs/simple_wmd.csv",
                                           "/pando/graphs/data.00001.csv"));

INSTANTIATE_TEST_SUITE_P(DISABLED_BigFiles, DLCSRInit,
                         ::testing::Values("/pando/graphs/data.001.csv",
                                           "/pando/graphs/data.005.csv",
                                           "/pando/graphs/data.01.csv"));

TEST(initializeWMD, edgelist) {
  Graph graph;
  pando::Array<char> filename;
  std::string wmdFile = "/pando/graphs/simple.el";
  bool isEdgelist = true;
  PANDO_CHECK(filename.initialize(wmdFile.size()));
  for (uint64_t i = 0; i < wmdFile.size(); i++)
    filename[i] = wmdFile[i];
  pando::GlobalPtr<Graph> dGraphPtr = static_cast<decltype(dGraphPtr)>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(Graph)));

  PANDO_CHECK(dGraphPtr->initializeWMD(filename, isEdgelist));
  liftVoid(*dGraphPtr, deinitialize);
}
