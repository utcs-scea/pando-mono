// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_
#define PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_

#include <pando-rt/export.h>

#include <cassert>
#include <cstdlib>
#include <utility>

#include <pando-lib-galois/utility/agile_schema.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>

namespace galois {

pando::Vector<galois::StringView> splitLine(const char* line, char delim, uint64_t numTokens);

template <std::uint64_t numTokens>
void splitLine(const char* line, char delim, pando::Array<galois::StringView> tokens) {
  assert(tokens.size() == numTokens);
  uint64_t ndx = 0, start = 0, end = 0;

  for (; line[end] != '\0' && line[end] != '\n' && ndx < numTokens; end++) {
    if (line[end] == delim) {
      tokens[ndx] = galois::StringView(line + start, end - start);
      start = end + 1;
      ndx++;
    }
  }
  if (ndx < numTokens) {
    tokens[numTokens - 1] = galois::StringView(line + start, end - start); // flush last token
  }
}

template <typename EdgeType>
struct ParsedEdges {
  ParsedEdges() : isEdge(false), has2Edges(false) {}
  explicit ParsedEdges(EdgeType edge_) : isEdge(true), has2Edges(false), edge1(edge_) {}
  explicit ParsedEdges(EdgeType edge1_, EdgeType edge2_)
      : isEdge(true), has2Edges(true), edge1(edge1_), edge2(edge2_) {}

  bool isEdge;
  bool has2Edges;
  EdgeType edge1;
  EdgeType edge2;
};

template <typename EdgeType>
class EdgeParser {
public:
  EdgeParser() = default;
  EdgeParser(pando::Array<char> filename_, ParsedEdges<EdgeType> (*edgeParser_)(const char*),
             char comment_ = '#')
      : filename(filename_), parser(edgeParser_), comment(comment_) {}

  pando::Array<char> filename;
  ParsedEdges<EdgeType> (*parser)(const char*);
  char comment = '#';
};

template <typename VertexType>
class VertexParser {
public:
  VertexParser() = default;
  VertexParser(pando::Array<char> filename_, VertexType (*vertexParser_)(const char*),
               char comment_ = '#')
      : filename(filename_), parser(vertexParser_), comment(comment_) {}

  pando::Array<char> filename;
  VertexType (*parser)(const char*);
  char comment = '#';
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_SCHEMA_HPP_
