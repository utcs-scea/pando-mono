// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_
#define PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_

#include <pando-rt/export.h>

#include <cstdlib>
#include <utility>

#include <pando-lib-galois/utility/agile_schema.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-rt/containers/vector.hpp>

namespace galois {

template <typename V, typename E>
struct ParsedGraphStructure {
  ParsedGraphStructure() : isNode(false), isEdge(false) {}
  explicit ParsedGraphStructure(V node_) : node(node_), isNode(true), isEdge(false) {}
  explicit ParsedGraphStructure(pando::Vector<E> edges_)
      : edges(edges_), isNode(false), isEdge(true) {}

  V node;
  pando::Vector<E> edges;
  bool isNode;
  bool isEdge;
};

template <typename V, typename E>
class FileParser {
public:
  virtual const pando::Vector<const char*>& getFiles() = 0;
  virtual ParsedGraphStructure<V, E> parseLine(const char* line) = 0;
  static pando::Vector<galois::StringView> splitLine(const char* line, char delim,
                                                     uint64_t numTokens) {
    uint64_t ndx = 0, start = 0, end = 0;
    pando::Vector<galois::StringView> tokens;
    PANDO_CHECK(tokens.initialize(numTokens));

    for (; line[end] != '\0'; end++) {
      if (line[end] == delim) {
        tokens[ndx] = galois::StringView(line + start, end - start);
        start = end + 1;
        ndx++;
      }
    }
    tokens[numTokens - 1] = galois::StringView(line + start, end - start); // flush last token
    return tokens;
  }
};

template <typename V, typename E>
class WMDParser : public FileParser<V, E> {
public:
  explicit WMDParser(pando::Vector<const char*> files) : csvFields_(10), files_(files) {}
  WMDParser(uint64_t csvFields, pando::Vector<galois::StringView> files)
      : csvFields_(csvFields), files_(files) {}

  const pando::Vector<const char*>& getFiles() override {
    return files_;
  }
  ParsedGraphStructure<V, E> parseLine(const char* line) override {
    pando::Vector<galois::StringView> tokens = this->splitLine(line, ',', csvFields_);
    galois::StringView token1(tokens[1]);
    galois::StringView token2(tokens[2]);
    galois::StringView token3(tokens[3]);
    galois::StringView token4(tokens[4]);
    galois::StringView token5(tokens[5]);
    galois::StringView token6(tokens[6]);

    bool isNode =
        tokens[0] == galois::StringView("Person") ||
        tokens[0] == galois::StringView("ForumEvent") || tokens[0] == galois::StringView("Forum") ||
        tokens[0] == galois::StringView("Publication") || tokens[0] == galois::StringView("Topic");

    if (isNode) {
      return ParsedGraphStructure<V, E>(V(tokens));
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
        return ParsedGraphStructure<V, E>();
      }
      pando::Vector<E> edges;
      PANDO_CHECK(edges.initialize(0));
      E edge(tokens);

      // insert inverse edges to the graph
      E inverseEdge = edge;
      inverseEdge.type = inverseEdgeType;
      std::swap(inverseEdge.src, inverseEdge.dst);
      std::swap(inverseEdge.srcType, inverseEdge.dstType);

      PANDO_CHECK(edges.pushBack(edge));
      PANDO_CHECK(edges.pushBack(inverseEdge));

      return ParsedGraphStructure<V, E>(edges);
    }
  }

private:
  uint64_t csvFields_;
  pando::Vector<const char*> files_;
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_
