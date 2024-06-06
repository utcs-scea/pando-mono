// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GRAPHS_GRAPHTYPES_HPP_
#define PANDO_WF1_GRAPHS_GRAPHTYPES_HPP_

#include <limits>

#include <pando-lib-galois/graphs/wmd_graph.hpp>

namespace galois {

constexpr const float NULL_TRUTH_VALUE = std::numeric_limits<float>::max();

struct VertexEmbedding {
public:
  VertexEmbedding()
      : id(NULL_GLOBAL_ID),
        edges(0),
        type(agile::TYPES::NONE),
        sid(NULL_GLOBAL_ID),
        groundTruth(NULL_TRUTH_VALUE) {}
  VertexEmbedding(uint64_t id_, agile::TYPES type_)
      : id(id_), edges(0), type(type_), sid(NULL_GLOBAL_ID), groundTruth(NULL_TRUTH_VALUE) {}
  explicit VertexEmbedding(pando::Array<galois::StringView> tokens)
      : edges(0), sid(NULL_GLOBAL_ID), groundTruth(NULL_TRUTH_VALUE) {
    if (tokens[0] == galois::StringView("Person")) {
      type = agile::TYPES::PERSON;
    } else if (tokens[0] == galois::StringView("ForumEvent")) {
      type = agile::TYPES::FORUMEVENT;
    } else if (tokens[0] == galois::StringView("Forum")) {
      type = agile::TYPES::FORUM;
    } else if (tokens[0] == galois::StringView("Publication")) {
      type = agile::TYPES::PUBLICATION;
    } else if (tokens[0] == galois::StringView("Topic")) {
      type = agile::TYPES::TOPIC;
    } else {
      PANDO_ABORT("INVALID VERTEX TYPE");
    }
    id = lift(tokens[static_cast<std::uint64_t>(type)], getU64);
  }
  explicit VertexEmbedding(const WMDVertex& v_)
      : id(v_.id),
        edges(v_.edges),
        type(v_.type),
        sid(NULL_GLOBAL_ID),
        groundTruth(NULL_TRUTH_VALUE) {}

  constexpr VertexEmbedding(VertexEmbedding&&) noexcept = default;
  constexpr VertexEmbedding(const VertexEmbedding&) noexcept = default;
  ~VertexEmbedding() = default;

  constexpr VertexEmbedding& operator=(const VertexEmbedding&) noexcept = default;
  constexpr VertexEmbedding& operator=(VertexEmbedding&&) noexcept = default;

  friend bool operator==(const VertexEmbedding& lhs, const VertexEmbedding& rhs) noexcept {
    return (lhs.id == rhs.id) && (lhs.edges == rhs.edges) && (lhs.type == rhs.type);
  }

  friend bool operator!=(const VertexEmbedding& lhs, const VertexEmbedding& rhs) noexcept {
    return !(lhs == rhs);
  }

  /**
   * @brief Set a subgraph local ID.
   */
  void SetSID(std::uint64_t sid) {
    this->sid = sid;
  }

  /**
   * @brief Get a subgraph local ID.
   */
  std::uint64_t GetSID() {
    return this->sid;
  }

  uint64_t id;
  uint64_t edges;
  agile::TYPES type;
  /// Vertex local ID in a subgraph
  uint64_t sid;
  /// Vertex feature embedding
  pando::Array<float> embedding;
  /// Vertex type ground truth given
  /// Note that this value is not necessarily its type
  /// since the type can be incontiguous while GNN encodes
  /// it to a contiguous range.
  float groundTruth;
};

} // namespace galois

#endif // PANDO_WF1_GRAPHS_GRAPHTYPES_HPP_
