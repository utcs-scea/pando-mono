// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_

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
  using EdgeRange = typename G::EdgeRange;
  using VertexRange = typename G::VertexRange;
  using VertexDataRange = typename G::VertexDataRange;
  using EdgeDataRange = typename G::EdgeDataRange;
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_GRAPH_TRAITS_HPP_
