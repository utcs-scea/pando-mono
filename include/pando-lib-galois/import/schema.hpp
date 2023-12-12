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

pando::Vector<galois::StringView> splitLine(const char* line, char delim, uint64_t numTokens);

template <typename V, typename E, typename VFunc, typename EFunc>
void genParse(std::uint64_t numFields, const char* line, VFunc vfunc, EFunc efunc) {
  pando::Vector<galois::StringView> tokens = splitLine(line, ',', numFields);
  galois::StringView token1(tokens[1]);
  galois::StringView token2(tokens[2]);
  galois::StringView token3(tokens[3]);
  galois::StringView token4(tokens[4]);
  galois::StringView token5(tokens[5]);
  galois::StringView token6(tokens[6]);

  bool isNode =
      tokens[0] == galois::StringView("Person") || tokens[0] == galois::StringView("ForumEvent") ||
      tokens[0] == galois::StringView("Forum") || tokens[0] == galois::StringView("Publication") ||
      tokens[0] == galois::StringView("Topic");

  if (isNode) {
    vfunc(V(tokens));
    return;
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
      return;
    }
    efunc(E(tokens), inverseEdgeType);
    return;
  }
}

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

// Inheritance is not supported, this is the expected API for any parsers passed to the importer
/**
 * template <typename V, typename E>
 * class FileParser {
 * public:
 *   virtual pando::GlobalPtr<pando::Vector<const char*>> getFiles() = 0;
 *   virtual ParsedGraphStructure<V, E> parseLine(const char*) = 0;
 * };
 */

template <typename V, typename E>
class WMDParser {
public:
  explicit WMDParser(pando::GlobalPtr<pando::Vector<const char*>> files)
      : csvFields_(10), files_(files) {}
  WMDParser(uint64_t csvFields, pando::GlobalPtr<pando::Vector<const char*>> files)
      : csvFields_(csvFields), files_(files) {}

  pando::GlobalPtr<pando::Vector<const char*>> getFiles() {
    return files_;
  }
  ParsedGraphStructure<V, E> parseLine(const char* line) {
    ParsedGraphStructure<V, E> pgs{};
    genParse<V, E>(
        csvFields_, line,
        [&pgs](V vertex) {
          pgs = ParsedGraphStructure<V, E>(vertex);
        },
        [&pgs](E edge, agile::TYPES inverseEdgeType) {
          pando::Vector<E> edges;
          PANDO_CHECK(edges.initialize(2));
          E inverseEdge = edge;
          inverseEdge.type = inverseEdgeType;
          std::swap(inverseEdge.src, inverseEdge.dst);
          std::swap(inverseEdge.srcType, inverseEdge.dstType);

          edges[0] = edge;
          edges[1] = inverseEdge;
          pgs = ParsedGraphStructure<V, E>(edges);
        });
    return pgs;
  }

private:
  uint64_t csvFields_;
  pando::GlobalPtr<pando::Vector<const char*>> files_;
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_
