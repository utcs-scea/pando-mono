// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_

#include <pando-rt/export.h>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>

namespace galois {

template <typename EdgeType>
struct GenericEdge {
  GenericEdge() = default;
  GenericEdge(uint64_t src_, uint64_t dst_, EdgeType data_) : src(src_), dst(dst_), data(data_) {}

  uint64_t src;
  uint64_t dst;
  EdgeType data;
};

/**
 * @brief This is a csr built upon a distributed arrays
 */
template <typename VertexType, typename EdgeType>
class DistArrayCSR {
public:
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = std::uint64_t;
  // This is an opaque edge type, and this could be either a graph topology edge ID
  // or could be a pointer depending on the graph type.
  using EdgeHandle = std::uint64_t;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using EdgeRange = DistArraySlice<EdgeHandle>;

  template <typename, typename>
  friend class DistArrayCSR;

  /**
   * @brief topology id and edge ranges in order to ensure proper depromotion for doAll inference
   */
  struct VertexInfo {
    VertexTopologyID lid;
    EdgeRange edges;
    operator VertexTopologyID() {
      return lid;
    }
    operator EdgeRange() {
      return edges;
    }
  };

  template <typename Graph, typename Projection, typename V, typename E>
  struct ProjectionState {
    ProjectionState() = default;
    ProjectionState(Graph oldGraph_, Projection projection_,
                    galois::PerThreadVector<V> projectedVertices_,
                    galois::PerThreadVector<GenericEdge<E>> projectedEdges_)
        : oldGraph(oldGraph_),
          projection(projection_),
          projectedVertices(projectedVertices_),
          projectedEdges(projectedEdges_) {}

    Graph oldGraph;
    Projection projection;
    galois::PerThreadVector<V> projectedVertices;
    galois::PerThreadVector<GenericEdge<E>> projectedEdges;
  };

  static_assert(!std::is_same<VertexTopologyID, EdgeRange>::value);

  /**
   * @brief Vertex Iterator since the graph itself is returned as the range
   */
  class VertexIt {
    DistArray<EdgeHandle> m_vertexEdgeOffsets;
    DistArray<VertexTopologyID> m_edgeDestinations;
    VertexTopologyID m_vertex;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::int64_t;
    using value_type = VertexInfo;

    VertexIt(DistArrayCSR& dacsr, VertexTopologyID vertex)
        : m_vertexEdgeOffsets(dacsr.vertexEdgeOffsets),
          m_edgeDestinations(dacsr.edgeDestinations),
          m_vertex(vertex) {}
    constexpr VertexIt() noexcept = default;
    constexpr VertexIt(VertexIt&&) noexcept = default;
    constexpr VertexIt(const VertexIt&) noexcept = default;
    ~VertexIt() = default;

    constexpr VertexIt& operator=(const VertexIt&) noexcept = default;
    constexpr VertexIt& operator=(VertexIt&&) noexcept = default;

    value_type operator*() const noexcept {
      std::uint64_t beg = (m_vertex == 0) ? 0 : m_vertexEdgeOffsets[m_vertex - 1];
      return VertexInfo{m_vertex,
                        EdgeRange(m_edgeDestinations, beg, m_vertexEdgeOffsets[m_vertex])};
    }

    value_type operator*() noexcept {
      std::uint64_t beg = (m_vertex == 0) ? 0 : m_vertexEdgeOffsets[m_vertex - 1];
      return VertexInfo{m_vertex,
                        EdgeRange(m_edgeDestinations, beg, m_vertexEdgeOffsets[m_vertex])};
    }

    VertexIt& operator++() {
      m_vertex++;
      return *this;
    }

    VertexIt operator++(int) {
      VertexIt tmp = *this;
      ++(*this);
      return tmp;
    }

    VertexIt& operator--() {
      m_vertex--;
      return *this;
    }

    VertexIt operator--(int) {
      VertexIt tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const VertexIt& a, const VertexIt& b) {
      return a.m_vertex == b.m_vertex && isSame(a.m_vertexEdgeOffsets, b.m_vertexEdgeOffsets) &&
             isSame(a.m_edgeDestinations, b.m_edgeDestinations);
    }

    friend bool operator!=(const VertexIt& a, const VertexIt& b) {
      return !(a == b);
    }

    friend pando::Place localityOf(VertexIt& a) {
      std::uint64_t beg = (a.m_vertex == 0) ? 0 : a.m_vertexEdgeOffsets[a.m_vertex - 1];
      pando::GlobalPtr<VertexTopologyID> ptr = &a.m_edgeDestinations[beg];
      return galois::localityOf(ptr);
    }
  };

  using VertexRange = DistArrayCSR<VertexType, EdgeType>;
  using VertexDataRange = DistArraySlice<VertexType>;
  using EdgeDataRange = DistArraySlice<EdgeType>;

  using iterator = VertexIt;

  constexpr DistArrayCSR() noexcept = default;
  constexpr DistArrayCSR(DistArrayCSR&&) noexcept = default;
  constexpr DistArrayCSR(const DistArrayCSR&) noexcept = default;
  ~DistArrayCSR() = default;

  constexpr DistArrayCSR& operator=(const DistArrayCSR&) noexcept = default;
  constexpr DistArrayCSR& operator=(DistArrayCSR&&) noexcept = default;

  /**
   * @brief Creates a DistArrayCSR from an explicit graph definition, intended only for tests
   *
   * @param[in] vertices This is a vector of vertex values.
   * @param[in] edges This is vector global src id, dst id, and edge data.
   */
  [[nodiscard]] pando::Status initialize(pando::Vector<VertexType> vertices,
                                         pando::Vector<GenericEdge<EdgeType>> edges) {
    numVertices = vertices.size();
    numEdges = edges.size();

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

    err = vertexEdgeOffsets.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      return err;
    }

    err = vertexTokenIDs.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
    }

    err = vertexData.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      return err;
    }

    err = edgeDestinations.initialize(vec.begin(), vec.end(), edges.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      return err;
    }

    err = edgeData.initialize(vec.begin(), vec.end(), edges.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      edgeDestinations.deinitialize();
      return err;
    }

    for (std::uint64_t vertex = 0; vertex < vertices.size(); vertex++) {
      vertexTokenIDs[vertex] = vertex;
      vertexData[vertex] = vertices[vertex];
    }

    std::uint64_t vertexCurr = 0;
    for (std::uint64_t i = 0; i < edges.size(); i++) {
      GenericEdge<EdgeType> edge = edges[i];
      edgeData[i] = edge.data;
      edgeDestinations[i] = edge.dst;
      if (edge.src != vertexCurr) {
        for (; vertexCurr < edge.src; vertexCurr++) {
          vertexEdgeOffsets[vertexCurr] = i;
        }
      }
    }
    vertexEdgeOffsets[vertices.size() - 1] = edges.size();
    vec.deinitialize();
    return err;
  }

  /**
   * @brief Creates a DistArrayCSR from an explicit graph definition, intended only for tests
   *
   * @param[in] vertices This is a vector of vertex values with TokenIDs in an `id` field.
   * @param[in] edges This is vector global src id, dst id, and edge data.
   * @param[in] orderedNonContiguousIDs Edges are ordered by vertex, but vertex IDs are not
   * contiguous
   */
  [[nodiscard]] pando::Status initialize(pando::Vector<VertexType> vertices,
                                         pando::Vector<GenericEdge<EdgeType>> edges,
                                         bool orderedNonContiguousIDs) {
    if (!orderedNonContiguousIDs) {
      PANDO_ABORT("illegal options given, read function description");
    }
    numVertices = vertices.size();
    numEdges = edges.size();
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

    err = vertexEdgeOffsets.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      return err;
    }

    err = vertexTokenIDs.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
    }

    err = vertexData.initialize(vec.begin(), vec.end(), vertices.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      return err;
    }

    err = edgeDestinations.initialize(vec.begin(), vec.end(), edges.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      return err;
    }

    err = edgeData.initialize(vec.begin(), vec.end(), edges.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      edgeDestinations.deinitialize();
      return err;
    }

    galois::HashTable<uint64_t, uint64_t> tokenToGlobalID;
    err = tokenToGlobalID.initialize(vertices.size() * 3 / 2);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      edgeDestinations.deinitialize();
      edgeData.deinitialize();
      return err;
    }

    for (std::uint64_t vertex = 0; vertex < vertices.size(); vertex++) {
      vertexTokenIDs[vertex] = vertex;
      vertexData[vertex] = vertices[vertex];
      VertexData data = vertices[vertex];
      PANDO_CHECK(tokenToGlobalID.put(data.id, vertex));
    }

    uint64_t v = 0;
    VertexData vertexCurr = vertexData[v];
    for (std::uint64_t i = 0; i < edges.size(); i++) {
      GenericEdge<EdgeType> edge = edges[i];
      edgeData[i] = edge.data;
      uint64_t localDst;
      if (!tokenToGlobalID.get(edge.dst, localDst)) {
        std::printf("failed destination vertex: %lu\n", edge.dst);
        PANDO_ABORT("given edge references a destination vertex that does not exist");
      }
      edgeDestinations[i] = localDst;
      for (; v < vertices.size() - 1 && edge.src != vertexCurr.id; v++) {
        vertexEdgeOffsets[v] = i;
        vertexCurr = vertexData[v + 1];
      }
      if (v == vertices.size() - 1 && edge.src != vertexCurr.id) {
        std::printf("failed source vertex: %lu, edge id: %lu\n", edge.src, i);
        PANDO_ABORT("given edge references a source vertex that does not exist");
      }
    }
    vertexEdgeOffsets[vertices.size() - 1] = edges.size();
    tokenToGlobalID.deinitialize();
    vec.deinitialize();
    return err;
  }

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

    err = vertexEdgeOffsets.initialize(vec.begin(), vec.end(), edgeList.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      return err;
    }

    err = vertexTokenIDs.initialize(vec.begin(), vec.end(), edgeList.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
    }

    err = vertexData.initialize(vec.begin(), vec.end(), edgeList.size());
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      return err;
    }

    std::uint64_t edgeNums = 0;
    for (pando::Vector<std::uint64_t> bucket : edgeList) {
      edgeNums += bucket.size();
    }
    numVertices = edgeList.size();
    numEdges = edgeNums;

    err = edgeDestinations.initialize(vec.begin(), vec.end(), edgeNums);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      return err;
    }

    err = edgeData.initialize(vec.begin(), vec.end(), edgeNums);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      vertexEdgeOffsets.deinitialize();
      vertexTokenIDs.deinitialize();
      vertexData.deinitialize();
      edgeDestinations.deinitialize();
      return err;
    }

    std::uint64_t edgeCurr = 0;
    for (std::uint64_t vertexCurr = 0; vertexCurr < edgeList.size(); vertexCurr++) {
      pando::Vector<std::uint64_t> edges = edgeList[vertexCurr];
      for (auto edgesIt = edges.cbegin(); edgesIt != edges.cend(); edgesIt++, edgeCurr++) {
        edgeDestinations[edgeCurr] = *edgesIt;
      }
      vertexEdgeOffsets[vertexCurr] = edgeCurr;
      vertexTokenIDs[vertexCurr] = vertexCurr;
    }

    if (edgeList.size() > 0 && vertexEdgeOffsets.size() > edgeList.size()) {
      // If a distributed array allocates more memory than the edge list size,
      // fill the vertex offsets since this graph returns size() of the length
      // of the vertex edge offset array, and it could cause an infinite loop
      // while edges are iterated through edgeBegin() and edgeEnd().
      for (std::uint64_t remainingIndex = edgeList.size();
           remainingIndex < vertexEdgeOffsets.size(); ++remainingIndex) {
        vertexEdgeOffsets[remainingIndex] = vertexEdgeOffsets[remainingIndex - 1];
        // TODO(hc): should we also take care of vertexTokenIDs?
      }
    }
    vec.deinitialize();
    return err;
  }

  /**
   * @brief Frees all memory and objects associated with this structure
   */
  void deinitialize() {
    vertexEdgeOffsets.deinitialize();
    vertexTokenIDs.deinitialize();
    edgeDestinations.deinitialize();
    vertexData.deinitialize();
    edgeData.deinitialize();
  }

  /**
   * @brief gives the number of vertices
   */
  std::uint64_t size() noexcept {
    return numVertices;
  }

  /**
   * @brief gives the number of vertices
   */
  std::uint64_t size() const noexcept {
    return numVertices;
  }

  /**
   * @brief get the token ID as input
   */
  VertexTokenID getTokenID(VertexTopologyID vertex) {
    return vertexTokenIDs[vertex];
  }

  /**
   * @brief get the topology ID as input
   *
   * @warning unimplemented
   */
  VertexTopologyID getTopologyID(VertexTokenID vertex) {
    return vertex;
  }

  /**
   * @brief Sets the value of the vertex provided
   */
  void setData(VertexTopologyID vertex, VertexData data) {
    vertexData[vertex] = data;
  }

  /**
   * @brief gets an edgeHandle from a vertex and offset
   */
  EdgeHandle mintEdgeHandle(VertexTopologyID vertex, std::uint64_t off) {
    std::uint64_t beg = (vertex == 0) ? 0 : vertexEdgeOffsets[vertex - 1];
    return beg + off;
  }

  /**
   * @brief gets the value of the vertex provided
   */
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return vertexData[vertex];
  }

  /**
   * @brief Sets the value of the edge provided
   */
  void setEdgeData(EdgeHandle eh, EdgeData data) {
    edgeData[eh] = data;
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
    return edgeData[eh];
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
    std::uint64_t beg = (vertex == 0) ? 0 : vertexEdgeOffsets[vertex - 1];
    std::uint64_t end = vertexEdgeOffsets[vertex];
    return end - beg;
  }

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  std::uint64_t getEdgeDst(EdgeHandle eh) {
    return edgeDestinations[eh];
  }

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  std::uint64_t getEdgeDst(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeDst(mintEdgeHandle(vertex, off));
  }

  pando::Place getLocalityVertex(VertexTopologyID vertex) {
    std::uint64_t beg = (vertex == 0) ? 0 : vertexEdgeOffsets[vertex - 1];
    return galois::localityOf(&edgeDestinations[beg]);
  }

  bool isLocal(VertexTopologyID vertex) {
    return getLocalityVertex(vertex).node == pando::getCurrentPlace().node;
  }

  bool isOwned(VertexTopologyID vertex) {
    return isLocal(vertex);
  }

  /**
   * @brief Project the graph given some Projection class
   * @warning This consumes the original graph
   * @note Tests for Project exist in
   * https://github.com/AMDResearch/PANDO-wf4-gal-root/blob/main/test/test_import.cpp
   */
  template <class OldGraph, class NewGraph, class Projection>
  NewGraph Project(Projection projection) {
    using NewVertexType = typename NewGraph::VertexData;
    using NewEdgeType = typename NewGraph::EdgeData;

    galois::PerThreadVector<NewVertexType> projectedVertices;
    galois::PerThreadVector<GenericEdge<NewEdgeType>> projectedEdges;
    PANDO_CHECK(projectedVertices.initialize());
    PANDO_CHECK(projectedEdges.initialize());
    ProjectionState state(*this, projection, projectedVertices, projectedEdges);

    galois::doAll(
        state, vertices(),
        +[](ProjectionState<OldGraph, Projection, NewVertexType, NewEdgeType>& state,
            VertexTopologyID node) {
          if (!state.projection.KeepNode(state.oldGraph, node)) {
            return;
          }
          uint64_t keptEdges = 0;
          for (EdgeHandle edge : state.oldGraph.edges(node)) {
            EdgeType edgeData = state.oldGraph.getEdgeData(edge);
            VertexTopologyID dstNode = state.oldGraph.getEdgeDst(edge);
            if (!state.projection.KeepEdge(state.oldGraph, edgeData, node, dstNode)) {
              continue;
            }
            keptEdges++;
            PANDO_CHECK(state.projectedEdges.pushBack(GenericEdge(
                node, dstNode,
                state.projection.ProjectEdge(state.oldGraph, edgeData, node, dstNode))));
          }
          if (state.projection.KeepEdgeLessMasters() || keptEdges > 0) {
            VertexType nodeData = state.oldGraph.getData(node);
            PANDO_CHECK(state.projectedVertices.pushBack(
                state.projection.ProjectNode(state.oldGraph, nodeData, node)));
          }
        });
    deinitialize();

    pando::GlobalPtr<pando::Vector<NewVertexType>> newVerticesPtr;
    pando::GlobalPtr<pando::Vector<GenericEdge<NewEdgeType>>> newEdgesPtr;
    auto expectVertices = pando::allocateMemory<pando::Vector<NewVertexType>>(
        1, pando::getCurrentPlace(), pando::MemoryType::Main);
    if (!expectVertices.hasValue()) {
      PANDO_ABORT("could not allocate pointer");
    }
    newVerticesPtr = expectVertices.value();
    auto expectEdges = pando::allocateMemory<pando::Vector<GenericEdge<NewEdgeType>>>(
        1, pando::getCurrentPlace(), pando::MemoryType::Main);
    if (!expectEdges.hasValue()) {
      PANDO_ABORT("could not allocate pointer");
    }
    newEdgesPtr = expectEdges.value();

    PANDO_CHECK(projectedVertices.assign(newVerticesPtr));
    PANDO_CHECK(projectedEdges.assign(newEdgesPtr));
    projectedVertices.deinitialize();
    projectedEdges.deinitialize();
    pando::Vector<NewVertexType> newVertices = *newVerticesPtr;
    pando::Vector<GenericEdge<NewEdgeType>> newEdges = *newEdgesPtr;
    // edge sources are sorted by construction due to no pre-emption
    std::printf("Projected vertices: %lu\n", newVertices.size());
    std::printf("Projected edges: %lu\n", newEdges.size());

    const bool orderedNonContiguousIDs = true;
    NewGraph newGraph;
    PANDO_CHECK(newGraph.initialize(newVertices, newEdges, orderedNonContiguousIDs));
    newVertices.deinitialize();
    newEdges.deinitialize();
    pando::deallocateMemory<pando::Vector<NewVertexType>>(newVerticesPtr, 1);
    pando::deallocateMemory<pando::Vector<GenericEdge<NewEdgeType>>>(newEdgesPtr, 1);
    return newGraph;
  }

  /**
   * @brief Get the Vertices range
   */
  VertexRange& vertices() {
    return *this;
  }

  /**
   * @brief Get the beginning of vertices
   */
  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  /**
   * @brief Get the beginning of vertices
   */
  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  /**
   * @brief Get the end of vertices
   */
  iterator end() noexcept {
    return iterator(*this, size());
  }

  /**
   * @brief Get the end of vertices
   */
  iterator end() const noexcept {
    return iterator(*this, size());
  }

  /**
   * @brief Get the EdgeRange for the graph
   */
  EdgeRange edges(VertexTopologyID vertex) noexcept {
    return *iterator(*this, vertex);
  }

  /**
   * @brief Return the start edge index assigned to the specified vertex.
   */
  EdgeHandle edgeBegin(VertexTopologyID v) {
    return (v == 0) ? 0 : vertexEdgeOffsets[v - 1];
  }

  /**
   * @brief Return the end edge index assigned to the specified vertex.
   */
  EdgeHandle edgeEnd(VertexTopologyID v) {
    return vertexEdgeOffsets[v];
  }

  /**
   * @brief Get the VertexDataRange for the graph
   */
  VertexDataRange vertexDataRange() noexcept {
    return VertexDataRange(vertexData, 0, size());
  }

  /**
   * @brief Get the EdgeDataRange for the graph
   */
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    auto beg = mintEdgeHandle(vertex, 0);
    auto end = vertexEdgeOffsets[vertex];
    return EdgeDataRange(edgeData, beg, end);
  }

private:
  ///@brief Stores the number of vertices, size may differ from DistArray sizes
  uint64_t numVertices;
  ///@brief Stores the number of edges, size may differ from DistArray sizes
  uint64_t numEdges;
  ///@brief Stores the vertex offsets
  galois::DistArray<EdgeHandle> vertexEdgeOffsets;
  ///@brief Stores the vertex gids
  galois::DistArray<VertexTokenID> vertexTokenIDs;
  ///@brief Stores the edge destinations
  galois::DistArray<VertexTopologyID> edgeDestinations;
  ///@brief Stores the data for each vertex
  galois::DistArray<VertexData> vertexData;
  ///@brief Stores the data for each edge
  galois::DistArray<EdgeData> edgeData;
};

} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_DIST_ARRAY_CSR_HPP_
