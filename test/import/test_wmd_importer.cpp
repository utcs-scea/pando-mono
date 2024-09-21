// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <numeric>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-rt/memory/memory_guard.hpp>

void getVerticesAndEdgesWMD(
    const std::string& filename, std::unordered_map<std::uint64_t, galois::WMDVertex>& vertices,
    std::unordered_map<std::uint64_t, std::vector<galois::WMDEdge>>& hashTable) {
  std::ifstream file(filename);
  std::string line;
  pando::Array<galois::StringView> tokens;
  PANDO_CHECK(tokens.initialize(10));
  while (std::getline(file, line)) {
    if (line.find("//") != std::string::npos || line.find("#") != std::string::npos) {
      continue;
    } else if (line.find("/*") != std::string::npos || line.find("*/") != std::string::npos) {
      continue;
    } else {
      const char* ptr = line.c_str();
      galois::splitLine<10>(ptr, ',', tokens);
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
    }
  }
  tokens.deinitialize();
}

void getVerticesAndEdgesEL(const std::string& filename, std::uint64_t numVertices,
                           std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>& graph) {
  std::ifstream file(filename);
  std::uint64_t src, dst;
  while (file >> src && file >> dst) {
    if (src < numVertices && dst < numVertices) {
      graph[src].push_back(dst);
    }
  }
  for (std::uint64_t i = 0; i < numVertices; i++) {
    if (graph.find(i) == graph.end()) {
      graph[i] = std::vector<std::uint64_t>();
    }
  }
  file.close();
}

class DLCSRInit : public ::testing::TestWithParam<const char*> {};

TEST_P(DLCSRInit, initializeWMD) {
  using ET = galois::WMDEdge;
  using VT = galois::WMDVertex;
  using Graph = galois::DistLocalCSR<VT, ET>;
  Graph graph;
  pando::Array<char> filename;
  std::string wmdFile = GetParam();
  PANDO_CHECK(filename.initialize(wmdFile.size()));
  for (uint64_t i = 0; i < wmdFile.size(); i++)
    filename[i] = wmdFile[i];

  auto dGraph = galois::initializeWMDDLCSR<galois::WMDVertex, galois::WMDEdge>(filename);

  // Validate
  std::unordered_map<std::uint64_t, std::vector<ET>> goldenTable;
  std::unordered_map<std::uint64_t, VT> goldenVertices;
  getVerticesAndEdgesWMD(wmdFile, goldenVertices, goldenTable);

  EXPECT_EQ(goldenVertices.size(), dGraph.size());
  // Iterate over vertices
  std::uint64_t vid = 0;

  typename Graph::VertexRange vertices = dGraph.vertices();
  for (typename Graph::VertexTopologyID vert : vertices) {
    EXPECT_EQ(vid, dGraph.getVertexIndex(vert));
    vid++;
    typename Graph::VertexTokenID id = dGraph.getTokenID(vert);

    typename Graph::VertexData vertex = dGraph.getData(vert);
    EXPECT_NE(goldenVertices.find(id), goldenVertices.end())
        << "Failed to get tok_id:" << id << "\t with index: " << (vid - 1);

    VT goldenVertex = goldenVertices[id];
    vertex = dGraph.getData(vert);
    EXPECT_EQ(goldenVertex.id, vertex.id);
    goldenVertex.id = 0;
    EXPECT_EQ(goldenVertex.type, vertex.type);
    goldenVertex.type = agile::TYPES::NONE;
    EXPECT_EQ(goldenVertex.edges, vertex.edges);
    goldenVertex.edges = 0;
    dGraph.setData(vert, goldenVertex);
    vertex = dGraph.getData(vert);
    EXPECT_EQ(0, vertex.id);
    EXPECT_EQ(agile::TYPES::NONE, vertex.type);
    EXPECT_EQ(0, vertex.edges);

    // Iterate over edges
    EXPECT_NE(goldenTable.find(id), goldenTable.end())
        << "Failed to find edges with tok_id:" << id << "\t with index: " << (vid - 1);
    std::vector<ET> goldenEdges = goldenTable[id];
    EXPECT_EQ(goldenEdges.size(), dGraph.getNumEdges(vert))
        << "Number of edges for tok_id: " << id << "\t with index: " << (vid - 1);

    for (typename Graph::EdgeHandle eh : dGraph.edges(vert)) {
      typename Graph::EdgeData eData = dGraph.getEdgeData(eh);

      typename Graph::VertexTokenID dstTok = dGraph.getTokenID(dGraph.getEdgeDst(eh));
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
      dGraph.setEdgeData(eh, goldenEdge);
      eData = dGraph.getEdgeData(eh);
      EXPECT_EQ(0, eData.src);
      EXPECT_EQ(0, eData.dst);
      EXPECT_EQ(agile::TYPES::NONE, eData.type);
      EXPECT_EQ(agile::TYPES::NONE, eData.srcType);
      EXPECT_EQ(agile::TYPES::NONE, eData.dstType);
    }
  }
  // deinitialize
  dGraph.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(SmallFiles, DLCSRInit,
                         ::testing::Values("/pando/graphs/simple_wmd.csv",
                                           "/pando/graphs/data.00001.csv"));

INSTANTIATE_TEST_SUITE_P(DISABLED_BigFiles, DLCSRInit,
                         ::testing::Values("/pando/graphs/data.001.csv",
                                           "/pando/graphs/data.005.csv",
                                           "/pando/graphs/data.01.csv"));

class DLCSRInitEdgeList : public ::testing::TestWithParam<std::tuple<const char*, std::uint64_t>> {
};
TEST_P(DLCSRInitEdgeList, initializeEL) {
  using ET = galois::ELEdge;
  using VT = galois::ELVertex;
  using Graph = galois::DistLocalCSR<VT, ET>;

  const std::string elFile = std::get<0>(GetParam());
  const std::uint64_t numVertices = std::get<1>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  Graph graph =
      galois::initializeELDLCSR<Graph, galois::ELVertex, galois::ELEdge>(filename, numVertices);

  // Validate
  std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> goldenTable;
  getVerticesAndEdgesEL(elFile, numVertices, goldenTable);
  EXPECT_EQ(goldenTable.size(), graph.size());

  // Iterate over vertices
  std::uint64_t vid = 0;

  PANDO_CHECK(galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
      graph.vertices(), +[](typename Graph::VertexTopologyID vert) {
        EXPECT_EQ(static_cast<std::uint64_t>(localityOf(vert).node.id),
                  pando::getCurrentPlace().node.id);
      }));

  for (typename Graph::VertexTopologyID vert : graph.vertices()) {
    EXPECT_EQ(vid, graph.getVertexIndex(vert));
    vid++;
    typename Graph::VertexTokenID srcTok = graph.getTokenID(vert);

    EXPECT_LT(srcTok, numVertices);

    typename Graph::VertexData vertexData = graph.getData(vert);
    EXPECT_EQ(srcTok, vertexData.id);

    VT dumbVertex = VT{numVertices};
    graph.setData(vert, dumbVertex);
    vertexData = graph.getData(vert);
    EXPECT_EQ(vertexData.id, numVertices);

    EXPECT_EQ(static_cast<std::uint64_t>(localityOf(vert).node.id),
              graph.getPhysicalHostID(srcTok));

    // Iterate over edges
    EXPECT_NE(goldenTable.find(srcTok), goldenTable.end())
        << "Failed to find edges with tok_id:" << srcTok << "\t with index: " << (vid - 1);
    std::vector<std::uint64_t> goldenEdges = goldenTable[srcTok];

    EXPECT_EQ(goldenEdges.size(), graph.getNumEdges(vert))
        << "Number of edges for tok_id: " << srcTok << "\t with index: " << (vid - 1);

    for (typename Graph::EdgeHandle eh : graph.edges(vert)) {
      typename Graph::EdgeData eData = graph.getEdgeData(eh);

      EXPECT_EQ(eData.src, srcTok);

      typename Graph::VertexTokenID dstTok = graph.getTokenID(graph.getEdgeDst(eh));
      EXPECT_EQ(eData.dst, dstTok);

      auto goldenEdgeIt = std::find(goldenEdges.begin(), goldenEdges.end(), dstTok);
      EXPECT_NE(goldenEdgeIt, goldenEdges.end())
          << "Unable to find edge with src_tok: " << srcTok << "\tand dst_tok: " << dstTok
          << "\tat vertex: " << (vid - 1);
      ET dumbEdge = ET{numVertices, numVertices};
      graph.setEdgeData(eh, dumbEdge);
      eData = graph.getEdgeData(eh);
      EXPECT_EQ(eData.src, numVertices);
      EXPECT_EQ(eData.dst, numVertices);
    }

    // Check edge lists are sorted when read from RMAT
    auto src_edges = graph.edges(vert);
    auto end_ptr = src_edges.end();
    end_ptr--;
    for (auto it = src_edges.begin(); it != end_ptr && it != src_edges.end(); it++) {
      typename Graph::VertexTopologyID dst0 = graph.getEdgeDst(*it);
      typename Graph::VertexTopologyID dst1 = graph.getEdgeDst(*(it + 1));
      EXPECT_LE(graph.getTokenID(dst0), graph.getTokenID(dst1));
    }
  }
  filename.deinitialize();
  graph.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, DLCSRInitEdgeList,
    ::testing::Values(std::make_tuple("/pando/graphs/simple.el", 10),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el",
                                      1024)));

INSTANTIATE_TEST_SUITE_P(
    DISABLED_BigFiles, DLCSRInitEdgeList,
    ::testing::Values(
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el", 2048),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el", 4096),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el", 8192),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el", 16384),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale15_nV32768_nE441929.el", 32768),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale16_nV65536_nE909846.el", 65536),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale17_nV131072_nE1864704.el", 131072),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale18_nV262144_nE3806162.el", 262144)));

class MirrorDLCSRInitEdgeList
    : public ::testing::TestWithParam<std::tuple<const char*, std::uint64_t>> {};
TEST_P(MirrorDLCSRInitEdgeList, initializeEL) {
  using ET = galois::ELEdge;
  using VT = galois::ELVertex;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;
  galois::HostLocalStorageHeap::HeapInit();
  const std::string elFile = std::get<0>(GetParam());
  const std::uint64_t numVertices = std::get<1>(GetParam());

  pando::Array<char> filename;
  EXPECT_EQ(pando::Status::Success, filename.initialize(elFile.size()));
  for (uint64_t i = 0; i < elFile.size(); i++)
    filename[i] = elFile[i];

  Graph graph =
      galois::initializeELDLCSR<Graph, galois::ELVertex, galois::ELEdge>(filename, numVertices);

  // Validate
  std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> goldenTable;
  getVerticesAndEdgesEL(elFile, numVertices, goldenTable);
  EXPECT_EQ(goldenTable.size(), graph.size());

  // Iterate over vertices
  std::uint64_t vid = 0;

  for (typename Graph::VertexTopologyID vert : graph.vertices()) {
    EXPECT_EQ(vid, graph.getVertexIndex(vert));
    vid++;
    typename Graph::VertexTokenID srcTok = graph.getTokenID(vert);

    EXPECT_LT(srcTok, numVertices);

    typename Graph::VertexData vertexData = graph.getData(vert);
    EXPECT_EQ(srcTok, vertexData.id);

    /*
    It is not valid to set data on remote side's mirror.
    It only need to write data only on local, or remote side's master.
    Currently there is no command to check if master on remote, such that only write data on local
    */
    if (graph.isLocal(vert)) {
      VT dumbVertex = VT{numVertices};
      graph.setData(vert, dumbVertex);
      vertexData = graph.getData(vert);
      EXPECT_EQ(vertexData.id, numVertices);
    }

    // Iterate over edges
    EXPECT_NE(goldenTable.find(srcTok), goldenTable.end())
        << "Failed to find edges with tok_id:" << srcTok << "\t with index: " << (vid - 1);
    std::vector<std::uint64_t> goldenEdges = goldenTable[srcTok];

    for (typename Graph::EdgeHandle eh : graph.edges(vert)) {
      typename Graph::EdgeData eData = graph.getEdgeData(eh);

      EXPECT_EQ(eData.src, srcTok);

      typename Graph::VertexTokenID dstTok = graph.getTokenID(graph.getEdgeDst(eh));
      EXPECT_EQ(eData.dst, dstTok);

      auto mirrorTopology = graph.getTopologyID(dstTok);
      auto masterTopology = graph.getGlobalTopologyID(dstTok);
      if (mirrorTopology != masterTopology) {
        // If global, and local have different value.
        // It means current one have mirror. Mirror is local, but master is not.
        ASSERT_TRUE(graph.isLocal(mirrorTopology));
        ASSERT_TRUE(!graph.isLocal(masterTopology));
        // Mirror must exist in mirror range.
        auto it = graph.getLocalMirrorRange();
        ASSERT_TRUE(*it.begin() <= mirrorTopology && mirrorTopology < *it.end());
      } else {
        // If I don't have mirror, that could be because it is in local, or never be a destination
        // from me.
        if (graph.isLocal(masterTopology)) {
          // If it is from me, it is in my master range.
          auto it = graph.getLocalMasterRange();
          ASSERT_TRUE(*it.begin() <= masterTopology && masterTopology < *it.end());
          // In mirror to master, this should never exist
        }
      }

      auto goldenEdgeIt = std::find(goldenEdges.begin(), goldenEdges.end(), dstTok);
      EXPECT_NE(goldenEdgeIt, goldenEdges.end())
          << "Unable to find edge with src_tok: " << srcTok << "\tand dst_tok: " << dstTok
          << "\tat vertex: " << (vid - 1);
      ET dumbEdge = ET{numVertices, numVertices};
      graph.setEdgeData(eh, dumbEdge);
      eData = graph.getEdgeData(eh);
      EXPECT_EQ(eData.src, numVertices);
      EXPECT_EQ(eData.dst, numVertices);
    }
  }
  graph.deinitialize();
}

INSTANTIATE_TEST_SUITE_P(
    SmallFiles, MirrorDLCSRInitEdgeList,
    ::testing::Values(std::make_tuple("/pando/graphs/simple.el", 10),
                      std::make_tuple("/pando/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el",
                                      1024)));

INSTANTIATE_TEST_SUITE_P(
    DISABLED_BigFiles, MirrorDLCSRInitEdgeList,
    ::testing::Values(
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el", 2048),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el", 4096),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el", 8192),
        std::make_tuple("/pando/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el", 16384)));
