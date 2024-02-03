// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_WMD_GRAPH_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_WMD_GRAPH_HPP_

#include <limits>

#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/utility/agile_schema.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/string_view.hpp>

namespace galois {

class WMDVertex;
class WMDEdge;
typedef galois::DistArrayCSR<WMDVertex, WMDEdge> WMDGraph;
const uint64_t NULL_GLOBAL_ID = std::numeric_limits<uint64_t>::max();

struct WMDVertex {
public:
  WMDVertex() {
    id = NULL_GLOBAL_ID;
    edges = 0;
    type = agile::TYPES::NONE;
  }
  WMDVertex(uint64_t id_, agile::TYPES type_) : id(id_), edges(0), type(type_) {}
  explicit WMDVertex(pando::Vector<galois::StringView> tokens) {
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
    edges = 0;
  }

  constexpr WMDVertex(WMDVertex&&) noexcept = default;
  constexpr WMDVertex(const WMDVertex&) noexcept = default;
  ~WMDVertex() = default;

  constexpr WMDVertex& operator=(const WMDVertex&) noexcept = default;
  constexpr WMDVertex& operator=(WMDVertex&&) noexcept = default;

  uint64_t id;
  uint64_t edges;
  agile::TYPES type;

  friend bool operator==(const WMDVertex& lhs, const WMDVertex& rhs) noexcept {
    return (lhs.id == rhs.id) && (lhs.edges == rhs.edges) && (lhs.type == rhs.type);
  }

  friend bool operator!=(const WMDVertex& lhs, const WMDVertex& rhs) noexcept {
    return !(lhs == rhs);
  }
};

struct WMDEdge {
public:
  WMDEdge() {
    src = NULL_GLOBAL_ID;
    dst = NULL_GLOBAL_ID;
    type = agile::TYPES::NONE;
    srcType = agile::TYPES::NONE;
    dstType = agile::TYPES::NONE;
  }
  WMDEdge(uint64_t src_, uint64_t dst_, agile::TYPES type_, agile::TYPES srcType_,
          agile::TYPES dstType_)
      : src(src_), dst(dst_), type(type_), srcType(srcType_), dstType(dstType_) {}
  explicit WMDEdge(pando::Vector<galois::StringView> tokens) {
    galois::StringView token0(tokens[0]);
    galois::StringView token1(tokens[1]);
    galois::StringView token2(tokens[2]);
    galois::StringView token3(tokens[3]);
    galois::StringView token4(tokens[4]);
    galois::StringView token5(tokens[5]);
    galois::StringView token6(tokens[6]);
    if (tokens[0] == galois::StringView("Sale")) {
      src = token1.getU64();
      dst = token2.getU64();
      type = agile::TYPES::SALE;
      srcType = agile::TYPES::PERSON;
      dstType = agile::TYPES::PERSON;
    } else if (tokens[0] == galois::StringView("Author")) {
      src = token1.getU64();
      type = agile::TYPES::AUTHOR;
      srcType = agile::TYPES::PERSON;
      if (!token3.empty()) {
        dst = token3.getU64();
        dstType = agile::TYPES::FORUM;
      } else if (!token4.empty()) {
        dst = token4.getU64();
        dstType = agile::TYPES::FORUMEVENT;
      } else if (!token5.empty()) {
        dst = token5.getU64();
        dstType = agile::TYPES::PUBLICATION;
      }
    } else if (tokens[0] == galois::StringView("Includes")) {
      src = token3.getU64();
      dst = token4.getU64();
      type = agile::TYPES::INCLUDES;
      srcType = agile::TYPES::FORUM;
      dstType = agile::TYPES::FORUMEVENT;
    } else if (tokens[0] == galois::StringView("HasTopic")) {
      dst = token6.getU64();
      type = agile::TYPES::HASTOPIC;
      dstType = agile::TYPES::TOPIC;
      if (!token3.empty()) {
        src = token3.getU64();
        srcType = agile::TYPES::FORUM;
      } else if (!token4.empty()) {
        src = token4.getU64();
        srcType = agile::TYPES::FORUMEVENT;
      } else if (!token5.empty()) {
        src = token5.getU64();
        srcType = agile::TYPES::PUBLICATION;
      }
    } else if (tokens[0] == galois::StringView("HasOrg")) {
      src = token5.getU64();
      dst = token6.getU64();
      type = agile::TYPES::HASORG;
      srcType = agile::TYPES::PUBLICATION;
      dstType = agile::TYPES::TOPIC;
    } else {
      src = token0.getU64();
      dst = token1.getU64();
      type = agile::TYPES::NONE;
      srcType = agile::TYPES::NONE;
      dstType = agile::TYPES::NONE;
    }
  }

  friend bool operator==(const galois::WMDEdge& a, const galois::WMDEdge& b) {
    return a.src == b.src && a.dst == b.dst && a.type == b.type && a.srcType == b.srcType &&
           a.dstType == b.dstType;
  }

  friend bool operator!=(const galois::WMDEdge& a, const galois::WMDEdge& b) {
    return !(a == b);
  }

  constexpr WMDEdge(WMDEdge&&) noexcept = default;
  constexpr WMDEdge(const WMDEdge&) noexcept = default;
  ~WMDEdge() = default;

  constexpr WMDEdge& operator=(const WMDEdge&) noexcept = default;
  constexpr WMDEdge& operator=(WMDEdge&&) noexcept = default;

  uint64_t src;
  uint64_t dst;
  agile::TYPES type;
  agile::TYPES srcType;
  agile::TYPES dstType;
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_WMD_GRAPH_HPP_
