// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF4_GALOIS_FULL_GRAPH_HPP_
#define PANDO_WF4_GALOIS_FULL_GRAPH_HPP_

#include <pando-rt/export.h>

#include <charconv>
#include <limits>
#include <utility>

#include <pando-lib-galois/import/schema.hpp>
#include <pando-rt/containers/vector.hpp>

#ifdef DIST_ARRAY_CSR
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#else
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#endif

namespace wf4 {

class FullNetworkNode;
class FullNetworkEdge;

#ifdef DIST_ARRAY_CSR
typedef galois::DistArrayCSR<FullNetworkNode, FullNetworkEdge> FullNetworkGraph;
#else
typedef galois::DistLocalCSR<FullNetworkNode, FullNetworkEdge> FullNetworkGraph;
#endif

class FullNetworkEdge {
public:
  FullNetworkEdge() = default;
  explicit FullNetworkEdge(uint64_t src_, uint64_t dst_, agile::TYPES type_, agile::TYPES srcType_,
                           agile::TYPES dstType_, double amount, uint64_t topic_)
      : src(src_),
        dst(dst_),
        type(type_),
        srcType(srcType_),
        dstType(dstType_),
        amount_(amount),
        weight_(0),
        topic(topic_) {}
  explicit FullNetworkEdge(pando::Vector<galois::StringView> tokens) {
    galois::StringView token1 = tokens[1];
    galois::StringView token2 = tokens[2];
    galois::StringView token3 = tokens[3];
    galois::StringView token7 = tokens[7];

    if (!token7.empty()) {
      std::from_chars(token7.get(), token7.get() + token7.size(), amount_);
    } else {
      amount_ = 0;
    }

    weight_ = 0;
    src = token1.getU64();
    dst = token2.getU64();
    if (!token3.empty()) {
      topic = token3.getU64();
    } else {
      topic = 0;
    }
  }
  FullNetworkEdge(agile::TYPES type_, pando::Vector<galois::StringView> tokens) : type(type_) {
    galois::StringView token0 = tokens[0];
    galois::StringView token1 = tokens[1];

    src = token0.getU64();
    dst = token1.getU64();
    const uint64_t half_max = std::numeric_limits<uint64_t>::max() / 2;

    if (type == agile::TYPES::USES) {
      srcType = agile::TYPES::PERSON;
      dstType = agile::TYPES::DEVICE;
      dst = half_max + (dst % half_max);
    } else if (type == agile::TYPES::FRIEND) {
      srcType = agile::TYPES::PERSON;
      dstType = agile::TYPES::PERSON;
    } else if (type == agile::TYPES::COMMUNICATION) {
      srcType = agile::TYPES::DEVICE;
      dstType = agile::TYPES::DEVICE;
      src = half_max + (src % half_max);
      dst = half_max + (dst % half_max);

      galois::StringView token2 = tokens[2];
      galois::StringView token3 = tokens[3];
      galois::StringView token4 = tokens[4];
      galois::StringView token5 = tokens[5];
      galois::StringView token6 = tokens[6];
      galois::StringView token7 = tokens[7];
      galois::StringView token8 = tokens[8];
      galois::StringView token9 = tokens[9];
      galois::StringView token10 = tokens[10];

      epoch_time = token2.getU64();
      duration = token3.getU64();
      protocol = token4.getU64();
      src_port = token5.getU64();
      dst_port = token6.getU64();
      src_packets = token7.getU64();
      dst_packets = token8.getU64();
      src_bytes = token9.getU64();
      dst_bytes = token10.getU64();
    }
    amount_ = 0;
    topic = 0;
  }

  friend bool operator==(const FullNetworkEdge& a, const FullNetworkEdge& b) {
    return a.src == b.src;
  }
  friend bool operator!=(const FullNetworkEdge& a, const FullNetworkEdge& b) {
    return a.src != b.src;
  }
  friend bool operator<(const FullNetworkEdge& a, const FullNetworkEdge& b) {
    return a.src < b.src;
  }

  uint64_t src = 0;
  uint64_t dst = 0;
  agile::TYPES type = agile::TYPES::SALE;
  agile::TYPES srcType = agile::TYPES::PERSON;
  agile::TYPES dstType = agile::TYPES::PERSON;
  double amount_ = 0;
  double weight_ = 0;
  uint64_t topic = 0;

  uint64_t epoch_time = 0;
  uint64_t duration = 0;
  uint64_t protocol = 0;
  uint64_t src_port = 0;
  uint64_t dst_port = 0;
  uint64_t src_packets = 0;
  uint64_t dst_packets = 0;
  uint64_t src_bytes = 0;
  uint64_t dst_bytes = 0;
};

struct FullNetworkNode {
public:
  FullNetworkNode() = default;
  FullNetworkNode(uint64_t id_, agile::TYPES type_)
      : id(id_), frequency_(0), sold_(0), bought_(0), desired_(0), type(type_), extra_data(0) {}
  explicit FullNetworkNode(uint64_t id_)
      : id(id_),
        frequency_(0),
        sold_(0),
        bought_(0),
        desired_(0),
        type(agile::TYPES::NONE),
        extra_data(0) {}
  explicit FullNetworkNode(pando::Vector<galois::StringView> tokens) {
    galois::StringView token1(tokens[1]);
    galois::StringView token2(tokens[2]);
    galois::StringView token3(tokens[3]);
    galois::StringView token4(tokens[4]);
    galois::StringView token5(tokens[5]);
    galois::StringView token6(tokens[6]);
    if (tokens[0] == galois::StringView("Person")) {
      id = token1.getU64();
      type = agile::TYPES::PERSON;
    } else if (tokens[0] == galois::StringView("ForumEvent")) {
      id = token4.getU64();
      type = agile::TYPES::FORUMEVENT;
    } else if (tokens[0] == galois::StringView("Forum")) {
      id = token3.getU64();
      type = agile::TYPES::FORUM;
    } else if (tokens[0] == galois::StringView("Publication")) {
      id = token5.getU64();
      type = agile::TYPES::PUBLICATION;
    } else if (tokens[0] == galois::StringView("Topic")) {
      id = token6.getU64();
      type = agile::TYPES::TOPIC;
    }
  }

  uint64_t id = 0;
  uint64_t frequency_ = 0; // ATOMIC number of occurrences in Reverse Reachable Sets
  double sold_ = 0;        // ATOMIC amount of coffee sold
  double bought_ = 0;      // ATOMIC amount of coffee bought  (>= coffee sold)
  double desired_ = 0;     // amount of coffee desired (>= coffee bought)

  agile::TYPES type = agile::TYPES::NONE;
  uint64_t extra_data = 0;
};

} // namespace wf4

#endif // PANDO_WF4_GALOIS_FULL_GRAPH_HPP_
