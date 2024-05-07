// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_WF2_GALOIS_GRAPH_DS_H_
#define PANDO_WF2_GALOIS_GRAPH_DS_H_

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "export.h"
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-rt/containers/vector.hpp>

namespace wf {

class WMDVertex;
class WMDEdge;
const uint64_t NULL_GLOBAL_ID = std::numeric_limits<uint64_t>::max();

class PersonVertex {
public:
  time_t trans_date;

  PersonVertex() {
    trans_date = 0;
  }

  explicit PersonVertex(pando::Array<galois::StringView>&) {}
};

class ForumEventVertex {
public:
  uint64_t forum;
  time_t date;

  ForumEventVertex() {
    forum = 0;
    date = 0;
  }

  explicit ForumEventVertex(pando::Array<galois::StringView>& tokens) {
    forum = galois::StringView(tokens[3]).getU64();
    date = galois::StringView(tokens[7]).getUSDate();
  }
};

class ForumVertex {
public:
  ForumVertex() {}

  explicit ForumVertex(pando::Array<galois::StringView>&) {}
};

class PublicationVertex {
public:
  time_t date;

  PublicationVertex() {
    date = 0;
  }

  explicit PublicationVertex(pando::Array<galois::StringView>& tokens) {
    date = galois::StringView(tokens[7]).getUSDate();
  }
};

class TopicVertex {
public:
  double lat;
  double lon;

  TopicVertex() {
    lat = 0.0;
    lon = 0.0;
  }

  explicit TopicVertex(pando::Array<galois::StringView>& tokens) {
    lat = galois::StringView(tokens[8]).getDouble();
    lon = galois::StringView(tokens[9]).getDouble();
  }
};

class NoneVertex {};

struct WMDVertex {
  union VertexUnion {
    PersonVertex person;
    ForumEventVertex forum_event;
    ForumVertex forum;
    PublicationVertex publication;
    TopicVertex topic;
    NoneVertex none;
    VertexUnion() {
      none = NoneVertex();
    }
  };

public:
  WMDVertex() {
    id = NULL_GLOBAL_ID;
    edges = 0;
    type = agile::TYPES::NONE;
  }
  WMDVertex(uint64_t id_, agile::TYPES type_) : id(id_), edges(0), type(type_) {}
  explicit WMDVertex(pando::Array<galois::StringView>& tokens) {
    if (tokens[0] == galois::StringView("Person")) {
      type = agile::TYPES::PERSON;
      v.person = PersonVertex(tokens);
    } else if (tokens[0] == galois::StringView("ForumEvent")) {
      type = agile::TYPES::FORUMEVENT;
      v.forum_event = ForumEventVertex(tokens);
    } else if (tokens[0] == galois::StringView("Forum")) {
      type = agile::TYPES::FORUM;
      v.forum = ForumVertex(tokens);
    } else if (tokens[0] == galois::StringView("Publication")) {
      type = agile::TYPES::PUBLICATION;
      v.publication = PublicationVertex(tokens);
    } else if (tokens[0] == galois::StringView("Topic")) {
      type = agile::TYPES::TOPIC;
      v.topic = TopicVertex(tokens);
    } else {
      PANDO_ABORT("INVALID VERTEX TYPE");
    }
    id = lift(tokens[static_cast<std::uint64_t>(type)], galois::StringView::getU64);
    edges = 0;
  }

  constexpr WMDVertex(WMDVertex&&) noexcept = default;
  constexpr WMDVertex(const WMDVertex&) noexcept = default;
  ~WMDVertex() = default;

  constexpr WMDVertex& operator=(const WMDVertex&) noexcept = default;
  constexpr WMDVertex& operator=(WMDVertex&&) noexcept = default;

  explicit WMDVertex(galois::WMDVertex& galoisWMDVertex) : WMDVertex(galoisWMDVertex.tokens) {}

  uint64_t id;
  uint64_t edges;
  agile::TYPES type;
  VertexUnion v;

  friend bool operator==(const WMDVertex& lhs, const WMDVertex& rhs) noexcept {
    return (lhs.id == rhs.id) && (lhs.edges == rhs.edges) && (lhs.type == rhs.type);
  }

  friend bool operator!=(const WMDVertex& lhs, const WMDVertex& rhs) noexcept {
    return !(lhs == rhs);
  }
};

class SaleEdge {
public:
  uint64_t seller;
  uint64_t buyer;
  uint64_t product;
  time_t date;

  SaleEdge() {
    seller = 0;
    buyer = 0;
    product = 0;
    date = 0;
  }

  explicit SaleEdge(pando::Array<galois::StringView>& tokens) {
    seller = galois::StringView(tokens[1]).getU64();
    buyer = galois::StringView(tokens[2]).getU64();
    product = galois::StringView(tokens[6]).getU64();
    date = galois::StringView(tokens[7]).getUSDate();
  }
};

class AuthorEdge {
public:
  uint64_t author;
  uint64_t item;

  AuthorEdge() {
    author = 0;
    item = 0;
  }

  explicit AuthorEdge(pando::Array<galois::StringView>& tokens) {
    if (!galois::StringView(tokens[3]).empty()) {
      author = galois::StringView(tokens[1]).getU64();
      item = galois::StringView(tokens[3]).getU64();
    } else if (!galois::StringView(tokens[4]).empty()) {
      author = galois::StringView(tokens[1]).getU64();
      item = galois::StringView(tokens[4]).getU64();
    } else {
      author = galois::StringView(tokens[1]).getU64();
      item = galois::StringView(tokens[5]).getU64();
    }
  }
};

class IncludesEdge {
public:
  uint64_t forum;
  uint64_t forum_event;

  IncludesEdge() {
    forum = 0;
    forum_event = 0;
  }

  explicit IncludesEdge(pando::Array<galois::StringView>& tokens) {
    forum = galois::StringView(tokens[3]).getU64();
    forum_event = galois::StringView(tokens[4]).getU64();
  }
};

class HasTopicEdge {
public:
  uint64_t item;
  uint64_t topic;

  HasTopicEdge() {
    item = 0;
    topic = 0;
  }

  explicit HasTopicEdge(pando::Array<galois::StringView>& tokens) {
    if (!galois::StringView(tokens[3]).empty()) {
      item = galois::StringView(tokens[3]).getU64();
      topic = galois::StringView(tokens[6]).getU64();
    } else if (!galois::StringView(tokens[4]).empty()) {
      item = galois::StringView(tokens[4]).getU64();
      topic = galois::StringView(tokens[6]).getU64();
    } else {
      item = galois::StringView(tokens[5]).getU64();
      topic = galois::StringView(tokens[6]).getU64();
    }
  }
};

class HasOrgEdge {
public:
  uint64_t publication;
  uint64_t organization;

  HasOrgEdge() {
    publication = 0;
    organization = 0;
  }

  explicit HasOrgEdge(pando::Array<galois::StringView>& tokens) {
    publication = galois::StringView(tokens[5]).getU64();
    organization = galois::StringView(tokens[6]).getU64();
  }
};

class NoneEdge {};

struct WMDEdge {
public:
  union EdgeUnion {
    SaleEdge sale;
    AuthorEdge author;
    IncludesEdge includes;
    HasTopicEdge has_topic;
    HasOrgEdge has_org;
    NoneEdge none;

    EdgeUnion() : none(NoneEdge()) {}
  };
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
  explicit WMDEdge(pando::Array<galois::StringView>& tokens) {
    galois::StringView token0(tokens[0]);
    galois::StringView token1(tokens[1]);
    galois::StringView token2(tokens[2]);
    galois::StringView token3(tokens[3]);
    galois::StringView token4(tokens[4]);
    galois::StringView token5(tokens[5]);
    galois::StringView token6(tokens[6]);
    if (tokens[0] == galois::StringView("Sale")) {
      e.sale = SaleEdge(tokens);
    } else if (tokens[0] == galois::StringView("Author")) {
      e.author = AuthorEdge(tokens);
    } else if (tokens[0] == galois::StringView("Includes")) {
      e.includes = IncludesEdge(tokens);
    } else if (tokens[0] == galois::StringView("HasTopic")) {
      e.has_topic = HasTopicEdge(tokens);
    } else if (tokens[0] == galois::StringView("HasOrg")) {
      e.has_org = HasOrgEdge(tokens);
    }
  }

  friend bool operator==(const WMDEdge& a, const WMDEdge& b) {
    return a.src == b.src && a.dst == b.dst && a.type == b.type && a.srcType == b.srcType &&
           a.dstType == b.dstType;
  }

  friend bool operator!=(const WMDEdge& a, const WMDEdge& b) {
    return !(a == b);
  }

  constexpr WMDEdge(WMDEdge&&) noexcept = default;
  constexpr WMDEdge(const WMDEdge&) noexcept = default;
  ~WMDEdge() = default;

  explicit WMDEdge(galois::WMDEdge galoisWMDEdge) : WMDEdge(galoisWMDEdge.tokens) {
    this->src = galoisWMDEdge.src;
    this->dst = galoisWMDEdge.dst;
    this->srcType = galoisWMDEdge.srcType;
    this->dstType = galoisWMDEdge.dstType;
    this->type = galoisWMDEdge.type;
  }

  constexpr WMDEdge& operator=(const WMDEdge&) noexcept = default;
  constexpr WMDEdge& operator=(WMDEdge&&) noexcept = default;

  uint64_t src;
  uint64_t dst;
  agile::TYPES type;
  agile::TYPES srcType;
  agile::TYPES dstType;
  EdgeUnion e;
};

} // namespace wf

#endif // PANDO_WF2_GALOIS_GRAPH_DS_H_
