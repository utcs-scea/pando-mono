// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_

#include <type_traits>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {

/**
 * @brief Defines the common interface for something to be considered a graph
 */
template <typename G>
struct graph_traits {
  using VertexTokenID = typename G::VertexTokenID;
  using VertexTopologyID = typename G::VertexTopologyID;
  using EdgeHandle = typename G::EdgeHandle;
  using VertexData = typename G::VertexData;
  using EdgeData = typename G::EdgeData;
  using VertexRange = typename G::VertexRange;
  using EdgeRange = typename G::EdgeRange;
  using VertexDataRange = typename G::VertexDataRange;
  using EdgeDataRange = typename G::EdgeDataRange;
};

/**
 * @brief this is the graph interface, methods from here should mostly be used
 */
template <typename G, typename VertexTokenID, typename VertexTopologyID, typename EdgeHandle,
          typename VertexData, typename EdgeData, typename VertexRange, typename EdgeRange,
          typename VertexDataRange, typename EdgeDataRange>
struct graph {
  void deinitialize();

  /** size stuff **/
  std::uint64_t size();
  std::uint64_t size() const noexcept;
  std::uint64_t sizeEdges();
  std::uint64_t sizeEdges() const noexcept;
  std::uint64_t getNumEdges(VertexTopologyID vertex);

  /** Vertex Manipulation **/
  VertexTopologyID getTopologyID(VertexTokenID token);
  VertexTopologyID getTopologyIDFromIndex(std::uint64_t index);
  VertexTokenID getTokenID(VertexTopologyID vertex);
  std::uint64_t getVertexIndex(VertexTopologyID vertex);
  pando::Place getLocalityVertex(VertexTopologyID vertex);

  /** Edge Manipulation **/
  EdgeHandle mintEdgeHandle(VertexTopologyID src, std::uint64_t off);
  VertexTopologyID getEdgeDst(EdgeHandle eh);

  /** Data Manipulations **/
  void setData(VertexTopologyID vertex, VertexData data);
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex);
  void setEdgeData(EdgeHandle eh, EdgeData data);
  pando::GlobalRef<EdgeData> getEdgeData(EdgeHandle eh);

  /** Ranges **/
  VertexRange vertices();
  EdgeRange edges(VertexTopologyID src);
  VertexDataRange vertexDataRange();
  EdgeDataRange edgeDataRange(VertexTopologyID vertex);

  /** Topology Modifications **/
  VertexTopologyID addVertexTopologyOnly(VertexTokenID token);
  VertexTopologyID addVertex(VertexTokenID token, VertexData data);
  pando::Status addEdgesTopologyOnly(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts);
  pando::Status addEdges(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts,
                         pando::Vector<EdgeData> data);
  pando::Status deleteEdges(VertexTopologyID src, pando::Vector<EdgeHandle> edges);
};

/**
 * @brief crazy sfinae enforcer to check that graph declares type
 * use it by doing `static_assert(graph_checker<Graph>::value);`
 */
template <typename G>
struct graph_checker {
  using GT = graph_traits<G>;

  using Yes = char[1];
  using No = char[2];

  /** Sizes **/
  static Yes& size(decltype(std::declval<G>().size())*);
  static No& size(...);
  static Yes& csize(decltype(std::declval<const G>().size())*);
  static No& csize(...);
  static Yes& sizeEdges(decltype(std::declval<G>().sizeEdges())*);
  static No& sizeEdges(...);
  static Yes& csizeEdges(decltype(std::declval<const G>().sizeEdges())*);
  static No& csizeEdges(...);
  static Yes& getNumEdges(
      decltype(std::declval<G>().getNumEdges(std::declval<typename GT::VertexTopologyID>()))*);
  static No& getNumEdges(...);

  /** Vertex Manipulation **/
  static Yes& getTopologyID(
      decltype(std::declval<G>().getTopologyID(std::declval<typename GT::VertexTokenID>()))*);
  static No& getTopologyID(...);
  static Yes& getTopologyIDFromIndex(
      decltype(std::declval<G>().getTopologyIDFromIndex(std::declval<std::uint64_t>()))*);
  static No& getTopologyIDFromIndex(...);
  static Yes& getTokenID(
      decltype(std::declval<G>().getTokenID(std::declval<typename GT::VertexTopologyID>()))*);
  static No& getTokenID(...);
  static Yes& getVertexIndex(
      decltype(std::declval<G>().getVertexIndex(std::declval<typename GT::VertexTopologyID>()))*);
  static No& getVertexIndex(...);
  static Yes& getLocalityVertex(decltype(std::declval<G>().getLocalityVertex(
      std::declval<typename GT::VertexTopologyID>()))*);
  static No& getLocalityVertex(...);

  /** Edge Manipulation **/
  static Yes& mintEdgeHandle(decltype(std::declval<G>().mintEdgeHandle(
      std::declval<typename GT::VertexTopologyID>(), std::declval<std::uint64_t>()))*);
  static No& mintEdgeHandle(...);
  static Yes& getEdgeDst(
      decltype(std::declval<G>().getEdgeDst(std::declval<typename GT::EdgeHandle>()))*);
  static No& getEdgeDst(...);

  /** Data Manipulations **/
  static Yes& setData(decltype(std::declval<G>().setData(
      std::declval<typename GT::VertexTopologyID>(), std::declval<typename GT::VertexData>()))*);
  static No& setData(...);
  static Yes& getData(
      decltype(std::declval<G>().getData(std::declval<typename GT::VertexTopologyID>()))*);
  static No& getData(...);
  static Yes& getEdgeData(
      decltype(std::declval<G>().getEdgeData(std::declval<typename GT::EdgeHandle>()))*);
  static No& getEdgeData(...);
  static Yes& setEdgeData(decltype(std::declval<G>().setEdgeData(
      std::declval<typename GT::EdgeHandle>(), std::declval<typename GT::EdgeData>()))*);
  static No& setEdgeData(...);

  /** Ranges **/
  static Yes& vertices(decltype(std::declval<G>().vertices())*);
  static No& vertices(...);
  static Yes& edges(
      decltype(std::declval<G>().edges(std::declval<typename GT::VertexTopologyID>()))*);
  static No& edges(...);
  static Yes& vertexDataRange(decltype(std::declval<G>().vertexDataRange())*);
  static No& vertexDataRange(...);
  static Yes& edgeDataRange(
      decltype(std::declval<G>().edgeDataRange(std::declval<typename GT::VertexTopologyID>()))*);
  static No& edgeDataRange(...);

  /** Topology Modifications **/
  static Yes& addVertexTopologyOnly(decltype(std::declval<G>().addVertexTopologyOnly(
      std::declval<typename GT::VertexTokenID>()))*);
  static No& addVertexTopologyOnly(...);
  static Yes& addVertex(decltype(std::declval<G>().addVertex(
      std::declval<typename GT::VertexTokenID>(), std::declval<typename GT::VertexData>()))*);
  static No& addVertex(...);
  static Yes& addEdgesTopologyOnly(decltype(std::declval<G>().addEdgesTopologyOnly(
      std::declval<typename GT::VertexTopologyID>(),
      std::declval<pando::Vector<typename GT::VertexTopologyID>>()))*);
  static No& addEdgesTopologyOnly(...);
  static Yes& addEdges(decltype(std::declval<G>().addEdges(
      std::declval<typename GT::VertexTopologyID>(),
      std::declval<pando::Vector<typename GT::VertexTopologyID>>(),
      std::declval<pando::Vector<typename GT::EdgeData>>()))*);
  static No& addEdges(...);
  static Yes& deleteEdges(decltype(std::declval<G>().deleteEdges(
      std::declval<typename GT::VertexTopologyID>(),
      std::declval<pando::Vector<typename GT::EdgeHandle>>()))*);
  static No& deleteEdges(...);

  static const bool value =
      sizeof(size(0)) == sizeof(Yes) && sizeof(csize(0)) == sizeof(Yes) &&
      sizeof(sizeEdges(0)) == sizeof(Yes) && sizeof(csizeEdges(0)) == sizeof(Yes) &&
      sizeof(getNumEdges(0)) == sizeof(Yes) && sizeof(getTopologyID(0)) == sizeof(Yes) &&
      sizeof(getTopologyIDFromIndex(0)) == sizeof(Yes) && sizeof(getTokenID(0)) == sizeof(Yes) &&
      sizeof(getVertexIndex(0)) == sizeof(Yes) && sizeof(getLocalityVertex(0)) == sizeof(Yes) &&
      sizeof(mintEdgeHandle(0)) == sizeof(Yes) && sizeof(getEdgeDst(0)) == sizeof(Yes) &&
      sizeof(setData(0)) == sizeof(Yes) && sizeof(getData(0)) == sizeof(Yes) &&
      sizeof(setEdgeData(0)) == sizeof(Yes) && sizeof(getEdgeData(0)) == sizeof(Yes) &&
      sizeof(vertices(0)) == sizeof(Yes) && sizeof(edges(0)) == sizeof(Yes) &&
      sizeof(vertexDataRange(0)) == sizeof(Yes) && sizeof(edgeDataRange(0)) == sizeof(Yes) &&
      sizeof(addVertexTopologyOnly(0)) == sizeof(Yes) && sizeof(addVertex(0)) == sizeof(Yes) &&
      sizeof(addEdgesTopologyOnly(0)) == sizeof(Yes) && sizeof(addEdges(0)) == sizeof(Yes) &&
      sizeof(deleteEdges(0)) == sizeof(Yes);
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_
