// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_
#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-rt/containers/vector.hpp>

namespace galois {
/**
 * @brief This is a csr built upon a distributed arrays
 */
template <typename NodeType, typename EdgeType>
struct DistArrayCSR {
  ///@brief Stores the vertex offsets
  galois::DistArray<std::uint64_t> nodeIndex;
  ///@brief Stores the edge destinations
  galois::DistArray<std::uint64_t> edgeIndex;
  ///@brief Stores the data for each node
  galois::DistArray<NodeType> nodeData;
  //@brief Stores the data for each edge
  galois::DistArray<EdgeType> edgeData;

  /**
   * @brief Creates a DistArrayCSR from an edgeList
   *
   * @param[in] edgeList This is an edgeList with the edges of each vertex.
   */
  pando::Status initialize(pando::Vector<pando::Vector<std::uint64_t>> edgeList) {
    pando::Status err;
    pando::Vector<galois::PlaceType> vec;
    err = vec.initialize(pando::getPlaceDims().node.id);
    if (err != pando::Status::Success) {
      return err;
    }

    for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
      vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                         pando::MemoryType::Main};
    }

    err = nodeIndex.initialize(vec.begin(), vec.end(), edgeList.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      return err;
    }

    err = nodeData.initialize(vec.begin(), vec.end(), edgeList.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      nodeIndex.deinitialize();
      return err;
    }

    std::uint64_t edgeNums = 0;
    for (pando::Vector<std::uint64_t> bucket : edgeList) {
      edgeNums += bucket.size();
    }

    err = edgeIndex.initialize(vec.begin(), vec.end(), edgeNums);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      nodeIndex.deinitialize();
      nodeData.deinitialize();
      return err;
    }

    err = edgeData.initialize(vec.begin(), vec.end(), edgeNums);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      nodeIndex.deinitialize();
      nodeData.deinitialize();
      edgeIndex.deinitialize();
      return err;
    }

    std::uint64_t edgeCurr = 0;
    for (std::uint64_t nodeCurr = 0; nodeCurr < edgeList.size(); nodeCurr++) {
      pando::Vector<std::uint64_t> edges = edgeList[nodeCurr];
      for (auto edgesIt = edges.cbegin(); edgesIt != edges.cend(); edgesIt++, edgeCurr++) {
        edgeIndex[edgeCurr] = *edgesIt;
      }
      nodeIndex[nodeCurr] = edgeCurr;
    }
    return err;
  }

  /**
   * @brief Frees all memory and objects associated with this structure
   */
  void deinitialize() {
    nodeIndex.deinitialize();
    edgeIndex.deinitialize();
    nodeData.deinitialize();
    edgeData.deinitialize();
  }

  /**
   * @brief gives the number of nodes
   */
  std::uint64_t size() noexcept {
    return nodeIndex.size();
  }

  /**
   * @brief Sets the value of the node provided
   */
  void setValue(std::uint64_t node, NodeType data) {
    nodeData[node] = data;
  }

  /**
   * @brief gets the value of the node provided
   */
  pando::GlobalRef<NodeType> getValue(std::uint64_t node) {
    return nodeData[node];
  }

  /**
   * @brief Sets the value of the edge provided
   */
  void setEdgeValue(std::uint64_t node, std::uint64_t off, EdgeType data) {
    std::uint64_t beg = (node == 0) ? 0 : nodeIndex[node - 1];
    edgeData[beg + off] = data;
  }

  /**
   * @brief gets the value of the node provided
   */
  pando::GlobalRef<EdgeType> getEdgeValue(std::uint64_t node, std::uint64_t off) {
    std::uint64_t beg = (node == 0) ? 0 : nodeIndex[node - 1];
    return edgeData[beg + off];
  }

  /**
   * @brief get the number of edges for the node provided
   */
  std::uint64_t getNumEdges(std::uint64_t node) {
    std::uint64_t beg = (node == 0) ? 0 : nodeIndex[node - 1];
    std::uint64_t end = nodeIndex[node];
    return end - beg;
  }

  /**
   * @brief get the vertex at the end of the edge provided by node at the offset from the start
   */
  std::uint64_t getEdgeDst(std::uint64_t node, std::uint64_t off) {
    std::uint64_t beg = (node == 0) ? 0 : nodeIndex[node - 1];
    return edgeIndex[beg + off];
  }

  /**
   * @brief Get the locality of a particular node
   */
  pando::Place getLocalityNode(std::uint64_t node) {
    std::uint64_t beg = (node == 0) ? 0 : nodeIndex[node - 1];
    return pando::localityOf(&edgeIndex[beg]);
  }
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_
