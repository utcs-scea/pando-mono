// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-wf4/import.hpp"

#include <algorithm>
#include <cstring>

#include <pando-lib-galois/graphs/projection.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-lib-galois/utility/timer.hpp>
#include <pando-rt/memory.hpp>
#include <pando-rt/pando-rt.hpp>

namespace {

struct ImportState {
  ImportState() = default;
  ImportState(galois::EdgeParser<wf4::FullNetworkEdge> parser_,
              galois::PerThreadVector<wf4::FullNetworkEdge> edges_)
      : parser(parser_), edges(edges_) {}

  galois::EdgeParser<wf4::FullNetworkEdge> parser;
  galois::PerThreadVector<wf4::FullNetworkEdge> edges;
};

void loadGraphFile(ImportState& state, uint64_t segmentID, uint64_t numSegments) {
  galois::ifstream graphFile;
  PANDO_CHECK(graphFile.open(state.parser.filename));
  uint64_t fileSize = graphFile.size();
  uint64_t bytesPerSegment = fileSize / numSegments;
  uint64_t start = segmentID * bytesPerSegment;
  uint64_t end = start + bytesPerSegment;

  pando::Vector<char> line;
  PANDO_CHECK(line.initialize(0));

  // check for partial line at start
  if (segmentID != 0) {
    graphFile.seekg(start - 1);
    graphFile.getline(line, '\n');

    // if not at start of a line, discard partial line
    if (!line.empty())
      start += line.size();
  }
  // check for partial line at end
  line.deinitialize();
  PANDO_CHECK(line.initialize(0));
  if (segmentID != numSegments - 1) {
    graphFile.seekg(end - 1);
    graphFile.getline(line, '\n');

    // if not at end of a line, include next line
    if (!line.empty())
      end += line.size();
  } else { // last locale processes to end of file
    end = fileSize;
  }
  line.deinitialize();
  graphFile.seekg(start);

  // load segment into memory
  uint64_t segmentLength = end - start;
  char* segmentBuffer = new char[segmentLength];
  graphFile.read(segmentBuffer, segmentLength);

  char* currentLine = segmentBuffer;
  char* endLine = currentLine + segmentLength;

  while (currentLine < endLine) {
    char* nextLine = std::strchr(currentLine, '\n') + 1;

    // skip comments
    if (currentLine[0] == '#') {
      currentLine = nextLine;
      continue;
    }
    galois::ParsedEdges<wf4::FullNetworkEdge> parsed = state.parser.parser(currentLine);
    if (parsed.isEdge) {
      PANDO_CHECK(state.edges.pushBack(parsed.edge1));
      if (parsed.has2Edges) {
        PANDO_CHECK(state.edges.pushBack(parsed.edge2));
      }
    }
    currentLine = nextLine;
  }
  delete[] segmentBuffer;
  graphFile.close();
}

} // namespace

wf4::FullNetworkGraph wf4::ImportData(InputFiles&& input_files) {
#ifdef DIST_ARRAY_CSR
  galois::DistArray<wf4::FullNetworkEdge> edges = internal::ImportFiles(input_files);
#endif

  galois::Timer initialize_timer("Start creating full graph", "Finished creating full graph");
  wf4::FullNetworkGraph full_graph{};
#ifdef DIST_ARRAY_CSR
  PANDO_CHECK(full_graph.initialize(std::move(edges)));
  input_files.deinitialize();
#else
  PANDO_CHECK(full_graph.initializeWMD(std::move(input_files)));
#endif
  initialize_timer.Stop();

  return full_graph;
}

wf4::NetworkGraph wf4::ProjectGraph(wf4::FullNetworkGraph full_graph) {
  return galois::Project<wf4::FullNetworkGraph, wf4::NetworkGraph,
                         wf4::internal::NetworkGraphProjection<wf4::FullNetworkGraph>>(
      std::move(full_graph), wf4::internal::NetworkGraphProjection<wf4::FullNetworkGraph>());
}

galois::DistArray<wf4::FullNetworkEdge> wf4::internal::ImportFiles(wf4::InputFiles input_files) {
  galois::Timer read_timer("Start reading input files", "Finished reading input files");
  // split files into roughly 10KB chunks
  const uint64_t chunk_size = 10000;
  galois::PerThreadVector<wf4::FullNetworkEdge> parsedEdges;
  PANDO_CHECK(parsedEdges.initialize());

  for (galois::EdgeParser<wf4::FullNetworkEdge> parser : input_files) {
    galois::ifstream graphFile;
    PANDO_CHECK(graphFile.open(parser.filename));
    uint64_t fileSize = graphFile.size();
    uint64_t segments = (fileSize / chunk_size) + 1;
    galois::doAllEvenlyPartition(ImportState(parser, parsedEdges), segments, &loadGraphFile);
  }
  read_timer.Stop();

  galois::Timer sort_timer("Start sorting read edges", "Finished sorting read edges");
  galois::DistArray<wf4::FullNetworkEdge> edges;
  PANDO_CHECK(parsedEdges.assign(edges));
  std::sort(edges.begin(), edges.end());
  sort_timer.Stop();

  parsedEdges.deinitialize();
  std::printf("Edges read: %lu\n", edges.size());
  return edges;
}

galois::ParsedEdges<wf4::FullNetworkEdge> wf4::ParseCommercialLine(const char* line) {
  pando::Vector<galois::StringView> tokens = galois::splitLine(line, ',', 8);

  bool isNode =
      tokens[0] == galois::StringView("Person") || tokens[0] == galois::StringView("ForumEvent") ||
      tokens[0] == galois::StringView("Forum") || tokens[0] == galois::StringView("Publication") ||
      tokens[0] == galois::StringView("Topic");

  if (isNode) {
    return galois::ParsedEdges<wf4::FullNetworkEdge>();
  } else { // edge type
    agile::TYPES inverseEdgeType;
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
      // skip nodes
      return galois::ParsedEdges<wf4::FullNetworkEdge>();
    }
    wf4::FullNetworkEdge edge(tokens);

    // insert inverse edges to the graph
    wf4::FullNetworkEdge inverseEdge = edge;
    inverseEdge.type = inverseEdgeType;
    std::swap(inverseEdge.src, inverseEdge.dst);
    std::swap(inverseEdge.srcType, inverseEdge.dstType);

    return galois::ParsedEdges<wf4::FullNetworkEdge>(edge, inverseEdge);
  }
}
galois::ParsedEdges<wf4::FullNetworkEdge> wf4::ParseCyberLine(const char* line) {
  pando::Vector<galois::StringView> tokens = galois::splitLine(line, ',', 11);
  wf4::FullNetworkEdge edge(agile::TYPES::COMMUNICATION, tokens);
  wf4::FullNetworkEdge inverseEdge = edge;
  inverseEdge.type = agile::TYPES::NONE;
  std::swap(inverseEdge.src, inverseEdge.dst);
  std::swap(inverseEdge.srcType, inverseEdge.dstType);
  return galois::ParsedEdges<wf4::FullNetworkEdge>(edge, inverseEdge);
}

galois::ParsedEdges<wf4::FullNetworkEdge> wf4::ParseSocialLine(const char* line) {
  pando::Vector<galois::StringView> tokens = galois::splitLine(line, ',', 2);
  wf4::FullNetworkEdge edge(agile::TYPES::FRIEND, tokens);
  wf4::FullNetworkEdge inverseEdge = edge;
  inverseEdge.type = agile::TYPES::NONE;
  std::swap(inverseEdge.src, inverseEdge.dst);
  std::swap(inverseEdge.srcType, inverseEdge.dstType);
  return galois::ParsedEdges<wf4::FullNetworkEdge>(edge, inverseEdge);
}

galois::ParsedEdges<wf4::FullNetworkEdge> wf4::ParseUsesLine(const char* line) {
  pando::Vector<galois::StringView> tokens = galois::splitLine(line, ',', 2);
  wf4::FullNetworkEdge edge(agile::TYPES::USES, tokens);
  wf4::FullNetworkEdge inverseEdge = edge;
  inverseEdge.type = agile::TYPES::NONE;
  std::swap(inverseEdge.src, inverseEdge.dst);
  std::swap(inverseEdge.srcType, inverseEdge.dstType);
  return galois::ParsedEdges<wf4::FullNetworkEdge>(edge, inverseEdge);
}
