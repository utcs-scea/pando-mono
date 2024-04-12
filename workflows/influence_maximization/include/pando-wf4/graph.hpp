// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF4_GRAPH_HPP_
#define PANDO_WF4_GRAPH_HPP_

#include "full_graph.hpp"
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace wf4 {

class NetworkNode;
class NetworkEdge;

#ifdef DIST_ARRAY_CSR
typedef galois::DistArrayCSR<NetworkNode, NetworkEdge> NetworkGraph;
#else
typedef galois::DistLocalCSR<NetworkNode, NetworkEdge> NetworkGraph;
#endif

struct NetworkNode {
public:
  NetworkNode() = default;

  [[nodiscard]] pando::Status initialize(uint64_t id_, pando::Place place,
                                         pando::MemoryType memoryType) {
    id = id_;

    const auto expected_u64 = pando::allocateMemory<std::uint64_t>(1, place, memoryType);
    if (!expected_u64.hasValue()) {
      return expected_u64.error();
    }
    frequency_ = expected_u64.value();
    *frequency_ = 0;

    const auto expected = pando::allocateMemory<double>(1, place, memoryType);
    if (!expected.hasValue()) {
      return expected.error();
    }
    sold_ = expected.value();
    *sold_ = 0;

    const auto expected_bought = pando::allocateMemory<double>(1, place, memoryType);
    if (!expected_bought.hasValue()) {
      return expected_bought.error();
    }
    bought_ = expected_bought.value();
    *bought_ = 0;

    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status initialize(const wf4::FullNetworkNode& full_node, pando::Place place,
                                         pando::MemoryType memoryType) {
    id = full_node.id;
    desired_ = full_node.desired_;

    const auto expected_u64 = pando::allocateMemory<std::uint64_t>(1, place, memoryType);
    if (!expected_u64.hasValue()) {
      return expected_u64.error();
    }
    frequency_ = expected_u64.value();
    *frequency_ = full_node.frequency_;

    const auto expected = pando::allocateMemory<double>(1, place, memoryType);
    if (!expected.hasValue()) {
      return expected.error();
    }
    sold_ = expected.value();
    *sold_ = full_node.sold_;

    const auto expected_bought = pando::allocateMemory<double>(1, place, memoryType);
    if (!expected_bought.hasValue()) {
      return expected_bought.error();
    }
    bought_ = expected_bought.value();
    *bought_ = full_node.bought_;

    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status initialize(uint64_t id) {
    return initialize(id, pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  [[nodiscard]] pando::Status initialize(const wf4::FullNetworkNode& full_node) {
    return initialize(full_node, pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  void deinitialize() {
    if (frequency_ != nullptr) {
      pando::deallocateMemory(frequency_, 1);
      frequency_ = nullptr;
    }
    if (sold_ != nullptr) {
      pando::deallocateMemory(sold_, 1);
      sold_ = nullptr;
    }
    if (bought_ != nullptr) {
      pando::deallocateMemory(bought_, 1);
      bought_ = nullptr;
    }
  }

  void Cancel() {
    *sold_ = 0;
    *bought_ = 0;
    desired_ = 0;
  }

  uint64_t id = 0;
  pando::GlobalPtr<uint64_t> frequency_ =
      nullptr; // ATOMIC number of occurrences in Reverse Reachable Sets
  pando::GlobalPtr<double> sold_ = nullptr;   // ATOMIC amount of coffee sold
  pando::GlobalPtr<double> bought_ = nullptr; // ATOMIC amount of coffee bought  (>= coffee sold)
  double desired_ = 0;                        // amount of coffee desired (>= coffee bought)
};

class NetworkEdge {
public:
  NetworkEdge() = default;
  NetworkEdge(double amount, agile::TYPES type_) : amount_(amount), weight_(0), type(type_) {}
  explicit NetworkEdge(const wf4::FullNetworkEdge& full_edge) {
    amount_ = full_edge.amount_;
    weight_ = full_edge.weight_;
    type = full_edge.type;
  }

  double amount_ = 0;
  double weight_ = 0;
  agile::TYPES type = agile::TYPES::NONE;
};

namespace internal {

struct WaitState {
  WaitState() = default;
  WaitState(NetworkGraph graph_, galois::WaitGroup::HandleType wgh_) : graph(graph_), wgh(wgh_) {}

  NetworkGraph graph;
  galois::WaitGroup::HandleType wgh;
};

} // namespace internal

} // namespace wf4

#endif // PANDO_WF4_GRAPH_HPP_
