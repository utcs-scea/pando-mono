// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-wf1/graphs/mhr_graph.hpp>

wf1::MHRNode wf1::ParseEmbeddingLine(const char* line) {
  return wf1::MHRNode(std::move(galois::splitLine(line, ',', EMBEDDING_FEATURE_SIZE + 1)));
}

galois::ParsedEdges<wf1::MHREdge> wf1::ParseRelationLine(const char* line) {
  pando::Vector<galois::StringView> tokens = galois::splitLine(line, ',', 4);
  wf1::MHREdge edge(std::move(tokens));
  wf1::MHREdge inverseEdge = edge;
  std::swap(inverseEdge.src, inverseEdge.dst);
  inverseEdge.mirror = true;
  return galois::ParsedEdges<wf1::MHREdge>(edge, inverseEdge);
}

[[nodiscard]] pando::Status wf1::RelationFeatures::initialize(
    galois::VertexParser<wf1::MHRNode>&& parser) {
  PANDO_CHECK_RETURN(features.initialize());
  for (uint64_t host = 0; host < features.size(); host++) {
    galois::HashTable<MHRGraph::VertexTokenID, pando::Vector<double>> localMap;
    PANDO_CHECK_RETURN(localMap.initialize(
        1000, pando::Place{pando::NodeIndex(host), pando::anyPod, pando::anyCore},
        pando::MemoryType::Main));
    features.get(host) = localMap;
  }
  pando::Status err = importFeatures(parser);
  if (err != pando::Status::Success) {
    deinitialize();
  }
  return err;
}

void wf1::RelationFeatures::deinitialize() {
  for (uint64_t host = 0; host < features.size(); host++) {
    galois::HashTable<MHRGraph::VertexTokenID, pando::Vector<double>> localMap = features.get(host);
    localMap.deinitialize();
  }
  features.deinitialize();
}

pando::Vector<double> wf1::RelationFeatures::getRelationFeature(
    wf1::MHRGraph::VertexTokenID relationID) {
  pando::Vector<double> relationFeatures;
  if (!fmap(features.getLocal(), get, relationID, relationFeatures)) {
    PANDO_ABORT("bad relation feature id lookup");
  }
  return relationFeatures;
}

[[nodiscard]] pando::Status wf1::RelationFeatures::importFeatures(
    galois::VertexParser<wf1::MHRNode>& parser) {
  galois::ifstream graphFile;
  PANDO_CHECK(graphFile.open(parser.filename));
  uint64_t fileSize = graphFile.size();
  uint64_t start = 0;
  uint64_t end = fileSize;
  graphFile.seekg(start);

  // load segment into memory
  uint64_t segmentLength = end - start;
  char* segmentBuffer = new char[segmentLength];
  graphFile.read(segmentBuffer, segmentLength);

  char* currentLine = segmentBuffer;
  char* endLine = currentLine + segmentLength;

  while (currentLine < endLine) {
    assert(std::strchr(currentLine, '\n'));
    char* nextLine = std::strchr(currentLine, '\n') + 1;

    // skip comments
    if (currentLine[0] == parser.comment) {
      currentLine = nextLine;
      continue;
    }

    wf1::MHRNode feature = parser.parser(currentLine);
    if (feature.id != AWARD_WINNER_TYPE && feature.id != WORKS_IN_TYPE &&
        feature.id != AFFILIATED_WITH_TYPE) {
      feature.deinitialize();
      currentLine = nextLine;
      continue;
    }
    for (uint64_t host = 0; host < features.size(); host++) {
      pando::Vector<double> localFeatures;
      PANDO_CHECK_RETURN(localFeatures.initialize(
          feature.features.size(),
          pando::Place{pando::NodeIndex(host), pando::anyPod, pando::anyCore},
          pando::MemoryType::Main));
      for (uint64_t i = 0; i < localFeatures.size(); i++) {
        localFeatures[i] = feature.features[i];
      }
      PANDO_CHECK_RETURN(fmap(features.get(host), put, feature.id, localFeatures));
    }
    feature.deinitialize();
    currentLine = nextLine;
  }
  delete[] segmentBuffer;
  graphFile.close();
  return pando::Status::Success;
}
