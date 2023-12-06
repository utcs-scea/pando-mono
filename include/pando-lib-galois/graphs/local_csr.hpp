// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_

#include <pando-rt/export.h>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {
struct Vertex;
struct HalfEdge {
  pando::GlobalPtr<Vertex> dst;
};

struct Vertex {
  pando::GlobalPtr<HalfEdge> edgeBegin;
};
} // namespace galois

// namespace pando {
//   template<typename>
//   GlobalRef<galois::Vertex>::operator Span<galois::HalfEdge>() {
//     GlobalPtr<Vertex> vPtr = &(*this);
//     Vertex v = *this;
//     Vertex v1 = *(vPtr + 1);
//     return Span<galois::HalfEdge>(v.edgeBegin, v1.edgeBegin - v.edgeBegin);
//   }
// } // namespace pando

namespace galois {

template <typename VertexType, typename EdgeType>
class LCSR {
public:
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = pando::GlobalRef<Vertex>;
  using EdgeHandle = pando::GlobalRef<HalfEdge>;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using VertexRange = pando::Span<Vertex>;
  using EdgeRange = pando::Span<HalfEdge>;
  using EdgeDataRange = pando::Span<EdgeData>;
  using VertexDataRange = pando::Span<VertexData>;

  constexpr LCSR() noexcept = default;
  constexpr LCSR(LCSR&&) noexcept = default;
  constexpr LCSR(const LCSR&) noexcept = default;
  ~LCSR() = default;

  constexpr LCSR& operator=(const LCSR&) noexcept = default;
  constexpr LCSR& operator=(LCSR&&) noexcept = default;

  static EdgeRange makeEdgeRange(pando::GlobalRef<galois::Vertex> vertex) {
    pando::GlobalPtr<Vertex> vPtr = &vertex;
    Vertex v = *vPtr;
    Vertex v1 = *(vPtr + 1);
    return pando::Span<galois::HalfEdge>(v.edgeBegin, v1.edgeBegin - v.edgeBegin);
  }

  /**
   * @brief initializes the memory and objects for a Vector based CSR
   */
  [[nodiscard]] pando::Status initialize(pando::Vector<pando::Vector<std::uint64_t>> edgeListCSR) {
    pando::Status err;
    err = vertexEdgeOffsets.initialize(edgeListCSR.size() + 1);
    if (err != pando::Status::Success) {
      return err;
    }

    err = vertexData.initialize(edgeListCSR.size());
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      return err;
    }

    std::uint64_t numEdges = 0;
    for (pando::Vector<std::uint64_t> v : edgeListCSR) {
      numEdges += v.size();
    }
    err = edgeData.initialize(numEdges);
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      vertexData.deinitialize();
      return err;
    }
    err = edgeDestinations.initialize(numEdges);
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      vertexData.deinitialize();
      edgeData.deinitialize();
      return err;
    }

    std::uint64_t edgeCurr = 0;
    vertexEdgeOffsets[0] = Vertex{edgeDestinations.begin()};
    for (std::uint64_t vertexCurr = 0; vertexCurr < edgeListCSR.size(); vertexCurr++) {
      pando::Vector<std::uint64_t> edges = edgeListCSR[vertexCurr];
      for (auto edgesIt = edges.cbegin(); edgesIt != edges.cend(); edgesIt++, edgeCurr++) {
        HalfEdge e;
        e.dst = &vertexEdgeOffsets[*edgesIt];
        edgeDestinations[edgeCurr] = e;
      }
      vertexEdgeOffsets[vertexCurr + 1] = Vertex{&edgeDestinations[edgeCurr]};
    }
    return pando::Status::Success;
  }

  /**
   * @brief Frees all memory and objects associated with this structure
   */
  void deinitialize() {
    vertexEdgeOffsets.deinitialize();
    edgeDestinations.deinitialize();
    vertexData.deinitialize();
    edgeData.deinitialize();
  }

  /**
   * @brief gives the number of vertices
   */
  std::uint64_t size() noexcept {
    return vertexEdgeOffsets.size() - 1;
  }

  /**
   * @brief gives the number of vertices
   */
  std::uint64_t size() const noexcept {
    return vertexEdgeOffsets.size() - 1;
  }

private:
  template <typename T>
  std::uint64_t findIndex(pando::GlobalRef<T> location, pando::Array<T> base) {
    pando::GlobalPtr<T> locationPtr = &location;
    if (base.begin() <= locationPtr && base.end() >= locationPtr) {
      return static_cast<uint64_t>(locationPtr - base.begin());
    } else {
      PANDO_ABORT("ILLEGAL SUBTRACTION OF POINTERS");
    }
  }

  EdgeHandle halfEdgeBegin(VertexTopologyID vertex) {
    return (&vertex == vertexEdgeOffsets.begin()) ? *edgeDestinations.begin()
                                                  : *static_cast<Vertex>(*(&vertex)).edgeBegin;
  }

  EdgeHandle halfEdgeEnd(VertexTopologyID vertex) {
    return *static_cast<Vertex>(*(&vertex + 1)).edgeBegin;
  }

public:
  /**
   * @brief Sets the value of the vertex provided
   */
  void setData(VertexTopologyID vertex, VertexData data) {
    vertexData[findIndex(vertex, vertexEdgeOffsets)] = data;
  }

  /**
   * @brief gets the value of the vertex provided
   */
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return vertexData[findIndex(vertex, vertexEdgeOffsets)];
  }

  /**
   * @brief gets an edgeHandle from a vertex and offset
   */
  EdgeHandle mintEdgeHandle(VertexTopologyID vertex, std::uint64_t off) {
    return *(&halfEdgeBegin(vertex) + off);
  }

  /**
   * @brief Sets the value of the edge provided
   */
  void setEdgeData(EdgeHandle eh, EdgeData data) {
    edgeData[findIndex(eh, edgeDestinations)] = data;
  }

  /**
   * @brief Sets the value of the edge provided
   */
  void setEdgeData(VertexTopologyID vertex, std::uint64_t off, EdgeData data) {
    setEdgeData(mintEdgeHandle(vertex, off), data);
  }

  /**
   * @brief gets the reference to the vertex provided
   */
  pando::GlobalRef<EdgeData> getEdgeData(EdgeHandle eh) {
    return edgeData[findIndex(eh, edgeDestinations)];
  }

  /**
   * @brief gets the reference to the vertex provided
   */
  pando::GlobalRef<EdgeData> getEdgeData(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeData(mintEdgeHandle(vertex, off));
  }

  /**
   * @brief get the number of edges for the vertex provided
   */
  std::uint64_t getNumEdges(VertexTopologyID vertex) {
    return halfEdgeEnd(vertex) - halfEdgeBegin(vertex);
  }

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  VertexTopologyID getEdgeDst(EdgeHandle eh) {
    HalfEdge e = eh;
    return *e.dst;
  }

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  VertexTopologyID getEdgeDst(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeDst(mintEdgeHandle(vertex, off));
  }

  pando::Place getLocalityVertex(VertexTopologyID vertex) {
    (void)vertex;
    return galois::localityOf(vertexEdgeOffsets.begin());
  }

  bool isLocal(VertexTopologyID vertex) {
    return getLocalityVertex(vertex).node == pando::getCurrentPlace().node;
  }

  bool isOwned(VertexTopologyID vertex) {
    return isLocal(vertex);
  }

  /**
   * @brief Get the Vertices range
   */
  VertexRange vertices() {
    return VertexRange(vertexEdgeOffsets.begin(), vertexEdgeOffsets.size() - 1);
  }

  /**
   * @brief Get the VertexDataRange for the graph
   */
  VertexDataRange vertexDataRange() noexcept {
    return VertexDataRange(vertexData.begin(), vertexData.size() - 1);
  }

  /**
   * @brief Get the EdgeDataRange for the graph
   */
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    auto beg = findIndex(halfEdgeBegin(vertex), edgeDestinations);
    auto end = findIndex(halfEdgeEnd(vertex), edgeDestinations);
    return EdgeDataRange(edgeData.begin() + beg, end - beg);
  }

private:
  pando::Array<Vertex> vertexEdgeOffsets;
  pando::Array<HalfEdge> edgeDestinations;
  pando::Array<VertexData> vertexData;
  pando::Array<EdgeData> edgeData;
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_
