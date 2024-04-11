// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_

#include <pando-rt/export.h>

#include <algorithm>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {
struct Vertex;
struct HalfEdge {
  pando::GlobalPtr<Vertex> dst;
};

struct Vertex {
  pando::GlobalPtr<HalfEdge> edgeBegin;
  uint64_t iterator_offset = 0;
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

template <typename T>
class PtrRef {
  pando::GlobalPtr<T> m_ptr;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = pando::GlobalRef<T>;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalPtr<T>;

  explicit PtrRef(pando::GlobalPtr<T> ptr) : m_ptr(ptr) {}
  constexpr PtrRef() noexcept = default;
  constexpr PtrRef(PtrRef&&) noexcept = default;
  constexpr PtrRef(const PtrRef&) noexcept = default;
  ~PtrRef() = default;

  constexpr PtrRef& operator=(const PtrRef&) noexcept = default;
  constexpr PtrRef& operator=(PtrRef&&) noexcept = default;

  reference operator*() const noexcept {
    return m_ptr;
  }

  reference operator*() noexcept {
    return m_ptr;
  }

  pointer operator->() {
    return m_ptr;
  }

  PtrRef& operator++() {
    m_ptr++;
    return *this;
  }

  PtrRef operator++(int) {
    PtrRef tmp = *this;
    ++(*this);
    return tmp;
  }

  PtrRef& operator--() {
    m_ptr--;
    return *this;
  }

  PtrRef operator--(int) {
    PtrRef tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr PtrRef operator+(std::uint64_t n) const noexcept {
    return PtrRef(m_ptr + n);
  }

  constexpr PtrRef& operator+=(std::uint64_t n) noexcept {
    m_ptr += n;
    return *this;
  }

  constexpr PtrRef operator-(std::uint64_t n) const noexcept {
    return PtrRef(m_ptr - n);
  }

  constexpr difference_type operator-(PtrRef b) const noexcept {
    return m_ptr - b.m_ptr;
  }

  reference operator[](std::uint64_t n) noexcept {
    return m_ptr + n;
  }

  reference operator[](std::uint64_t n) const noexcept {
    return m_ptr + n;
  }

  friend bool operator==(const PtrRef& a, const PtrRef& b) {
    return a.m_ptr == b.m_ptr;
  }

  friend bool operator!=(const PtrRef& a, const PtrRef& b) {
    return !(a == b);
  }

  friend bool operator<(const PtrRef& a, const PtrRef& b) {
    return a.m_ptr < b.m_ptr;
  }

  friend bool operator<=(const PtrRef& a, const PtrRef& b) {
    return a.m_ptr <= b.m_ptr;
  }

  friend bool operator>(const PtrRef& a, const PtrRef& b) {
    return a.m_ptr > b.m_ptr;
  }

  friend bool operator>=(const PtrRef& a, const PtrRef& b) {
    return a.m_ptr >= b.m_ptr;
  }

  friend pando::Place localityOf(PtrRef& a) {
    return pando::localityOf(a.m_ptr);
  }
};

template <typename T>
class RefSpan {
  pando::GlobalPtr<T> m_data{};
  std::uint64_t m_size{};

public:
  using iterator = PtrRef<T>;
  using const_iterator = PtrRef<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr RefSpan() noexcept = default;

  constexpr RefSpan(pando::GlobalPtr<T> data, std::uint64_t size) noexcept
      : m_data(data), m_size(size) {}

  constexpr std::uint64_t size() const noexcept {
    return m_size;
  }

  iterator begin() noexcept {
    return iterator(m_data);
  }

  iterator begin() const noexcept {
    return iterator(m_data);
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(m_data);
  }

  iterator end() noexcept {
    return iterator(m_data + size());
  }

  iterator end() const noexcept {
    return iterator(m_data + size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(m_data + size());
  }

  /**
   * @brief reverse iterator to the first element
   */
  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end()--);
  }

  /**
   * @copydoc rbegin()
   */
  reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end()--);
  }

  /**
   * @copydoc rbegin()
   */
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend()--);
  }

  /**
   * reverse iterator to the last element
   */
  reverse_iterator rend() noexcept {
    return reverse_iterator(begin()--);
  }

  /**
   * @copydoc rend()
   */
  reverse_iterator rend() const noexcept {
    return reverse_iterator(begin()--);
  }

  /**
   * @copydoc rend()
   */
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin()--);
  }
};

template <typename VertexType, typename EdgeType>
class DistLocalCSR;

template <typename VertexType, typename EdgeType>
class MirrorDistLocalCSR;

template <typename VertexType, typename EdgeType>
class LCSR {
public:
  friend DistLocalCSR<VertexType, EdgeType>;
  friend MirrorDistLocalCSR<VertexType, EdgeType>;
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = pando::GlobalPtr<Vertex>;
  using EdgeHandle = pando::GlobalPtr<HalfEdge>;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using VertexRange = RefSpan<Vertex>;
  using EdgeRange = RefSpan<HalfEdge>;
  using EdgeDataRange = pando::Span<EdgeData>;
  using VertexDataRange = pando::Span<VertexData>;

  constexpr LCSR() noexcept = default;
  constexpr LCSR(LCSR&&) noexcept = default;
  constexpr LCSR(const LCSR&) noexcept = default;
  ~LCSR() = default;

  constexpr LCSR& operator=(const LCSR&) noexcept = default;
  constexpr LCSR& operator=(LCSR&&) noexcept = default;

  [[nodiscard]] pando::Status initializeTopologyMemory(std::uint64_t numVertices,
                                                       std::uint64_t numEdges, pando::Place place,
                                                       pando::MemoryType memType) {
    pando::Status err;
    err = vertexEdgeOffsets.initialize(numVertices + 1, place, memType);
    if (err != pando::Status::Success) {
      return err;
    }

    err = topologyToToken.initialize(numVertices, place, memType);
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      return err;
    }
    err = tokenToTopology.initialize(numVertices, place, memType);
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      topologyToToken.deinitialize();
      return err;
    }
    err = edgeDestinations.initialize(numEdges, place, memType);
    if (err != pando::Status::Success) {
      vertexEdgeOffsets.deinitialize();
      topologyToToken.deinitialize();
      tokenToTopology.deinitialize();
      return err;
    }
    return err;
  }

  [[nodiscard]] pando::Status initializeDataMemory(std::uint64_t numVertices,
                                                   std::uint64_t numEdges, pando::Place place,
                                                   pando::MemoryType memType) {
    pando::Status err;
    err = vertexData.initialize(numVertices, place, memType);
    if (err != pando::Status::Success) {
      return err;
    }
    err = edgeData.initialize(numEdges, place, memType);
    if (err != pando::Status::Success) {
      vertexData.deinitialize();
      return err;
    }
    return err;
  }

  [[nodiscard]] pando::Status initializeTopologyMemory(std::uint64_t numVertices,
                                                       std::uint64_t numEdges) {
    return initializeTopologyMemory(numVertices, numEdges, pando::getCurrentPlace(),
                                    pando::MemoryType::Main);
  }

  [[nodiscard]] pando::Status initializeDataMemory(std::uint64_t numVertices,
                                                   std::uint64_t numEdges) {
    return initializeDataMemory(numVertices, numEdges, pando::getCurrentPlace(),
                                pando::MemoryType::Main);
  }

  /**
   * @brief initializes the memory and objects for a Vector based CSR
   */
  [[nodiscard]] pando::Status initialize(pando::Vector<pando::Vector<std::uint64_t>> edgeListCSR) {
    pando::Status err;
    std::uint64_t numEdges = 0;
    for (pando::Vector<std::uint64_t> v : edgeListCSR) {
      numEdges += v.size();
    }

    err = this->initializeTopologyMemory(edgeListCSR.size(), numEdges);
    if (err != pando::Status::Success) {
      return err;
    }
    err = this->initializeDataMemory(edgeListCSR.size(), numEdges);
    if (err != pando::Status::Success) {
      this->deinitializeTopology();
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
   * @brief Frees all memory and objects associated with the topology
   */
  void deinitializeTopology() {
    vertexEdgeOffsets.deinitialize();
    edgeDestinations.deinitialize();
    topologyToToken.deinitialize();
    tokenToTopology.deinitialize();
  }

  /**
   * @brief Frees all memory and objects associated with the data
   */
  void deinitializeData() {
    edgeData.deinitialize();
    vertexData.deinitialize();
  }

private:
  template <typename T>
  std::uint64_t findIndex(pando::GlobalPtr<T> locationPtr, pando::Array<T> base) {
    if (base.begin() <= locationPtr && base.end() >= locationPtr) {
      return static_cast<uint64_t>(locationPtr - base.begin());
    } else {
      PANDO_ABORT("ILLEGAL SUBTRACTION OF POINTERS");
    }
  }

  EdgeHandle halfEdgeBegin(VertexTopologyID vertex) {
    return (vertex == vertexEdgeOffsets.begin()) ? edgeDestinations.begin()
                                                 : static_cast<Vertex>(*(vertex)).edgeBegin;
  }

  EdgeHandle halfEdgeEnd(VertexTopologyID vertex) {
    return static_cast<Vertex>(*(vertex + 1)).edgeBegin;
  }

public:
  /** Graph APIs **/
  void deinitialize() {
    deinitializeTopology();
    deinitializeData();
  }

  /** size stuff **/
  std::uint64_t size() noexcept {
    return vertexEdgeOffsets.size() - 1;
  }
  std::uint64_t size() const noexcept {
    return vertexEdgeOffsets.size() - 1;
  }
  std::uint64_t sizeEdges() {
    return edgeDestinations.size();
  }
  std::uint64_t sizeEdges() const noexcept {
    return edgeDestinations.size();
  }
  std::uint64_t getNumEdges(VertexTopologyID vertex) {
    return halfEdgeEnd(vertex) - halfEdgeBegin(vertex);
  }

  /** Vertex Manipulation **/
private:
  // Use with your own risk.
  // It is reasonable only when you could handle the non-existing value outside of this function.
  galois::Pair<VertexTopologyID, bool> relaxedgetTopologyID(VertexTokenID token) {
    pando::GlobalPtr<Vertex> ret;
    bool found = tokenToTopology.get(token, ret);
    return galois::make_tpl(ret, found);
  }

public:
  VertexTopologyID getTopologyID(VertexTokenID token) {
    pando::GlobalPtr<Vertex> ret;
    if (!tokenToTopology.get(token, ret)) {
      std::cerr << "In the host " << pando::getCurrentPlace().node.id
                << "can't find token:" << token << std::endl;
      PANDO_ABORT("FAILURE TO FIND TOKENID");
    }
    return ret;
  }
  VertexTopologyID getTopologyIDFromIndex(std::uint64_t index) {
    return &vertexEdgeOffsets[index];
  }
  std::uint64_t getTokenID(VertexTopologyID vertex) {
    return topologyToToken[findIndex(vertex, vertexEdgeOffsets)];
  }
  std::uint64_t getVertexIndex(VertexTopologyID vertex) {
    return findIndex(vertex, vertexEdgeOffsets);
  }
  pando::Place getLocalityVertex(VertexTopologyID vertex) {
    return galois::localityOf(vertex);
  }

  /** Edge Manipulation **/
  EdgeHandle mintEdgeHandle(VertexTopologyID vertex, std::uint64_t off) {
    return halfEdgeBegin(vertex) + off;
  }
  VertexTopologyID getEdgeDst(EdgeHandle eh) {
    HalfEdge e = *eh;
    return e.dst;
  }

  /** Data Manipulation **/
  void setData(VertexTopologyID vertex, VertexData data) {
    vertexData[findIndex(vertex, vertexEdgeOffsets)] = data;
  }
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return vertexData[findIndex(vertex, vertexEdgeOffsets)];
  }
  void setEdgeData(EdgeHandle eh, EdgeData data) {
    edgeData[findIndex(eh, edgeDestinations)] = data;
  }
  pando::GlobalRef<EdgeData> getEdgeData(EdgeHandle eh) {
    return edgeData[findIndex(eh, edgeDestinations)];
  }

  /** Ranges **/
  VertexRange vertices() {
    return VertexRange(vertexEdgeOffsets.begin(), vertexEdgeOffsets.size() - 1);
  }

  VertexRange vertices(uint64_t offset_st, uint64_t window_sz) {
    auto beg = vertexEdgeOffsets.begin() + offset_st;
    auto sz = vertexEdgeOffsets.size() - 1;
    if (offset_st >= sz)
      return VertexRange(vertexEdgeOffsets.begin(), 0);

    return VertexRange(beg, std::min(window_sz, vertexEdgeOffsets.size() - 1 - offset_st));
  }

  static EdgeRange edges(pando::GlobalPtr<galois::Vertex> vPtr) {
    Vertex v = *vPtr;
    Vertex v1 = *(vPtr + 1);
    return EdgeRange(v.edgeBegin, v1.edgeBegin - v.edgeBegin);
  }

  static EdgeRange edges(pando::GlobalPtr<galois::Vertex> vPtr, uint64_t offset_st,
                         uint64_t window_sz) {
    Vertex v = *vPtr;
    Vertex v1 = *(vPtr + 1);

    auto beg = v.edgeBegin + offset_st;
    if (beg > v1.edgeBegin)
      return EdgeRange(v.edgeBegin, 0);

    auto clipped_window_sz = std::min(window_sz, (uint64_t)(v1.edgeBegin - beg));
    return EdgeRange(beg, clipped_window_sz);
  }

  VertexDataRange vertexDataRange() noexcept {
    return VertexDataRange(vertexData.begin(), vertexData.size() - 1);
  }
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    auto beg = findIndex(halfEdgeBegin(vertex), edgeDestinations);
    auto end = findIndex(halfEdgeEnd(vertex), edgeDestinations);
    return EdgeDataRange(edgeData.begin() + beg, end - beg);
  }

  /** Topology Modifications **/
  VertexTopologyID addVertexTopologyOnly(VertexTokenID token) {
    return vertexEdgeOffsets.end();
  }
  VertexTopologyID addVertex(VertexTokenID token, VertexData data) {
    return vertexEdgeOffsets.end();
  }
  pando::Status addEdgesTopologyOnly(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts) {
    return pando::Status::Error;
  }
  pando::Status addEdges(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts,
                         pando::Vector<EdgeData> data) {
    return pando::Status::Error;
  }
  pando::Status deleteEdges(VertexTopologyID src, pando::Vector<EdgeHandle> edges) {
    return pando::Status::Error;
  }

  /** End Graph API **/

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  VertexTopologyID getEdgeDst(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeDst(mintEdgeHandle(vertex, off));
  }

  bool isLocal(VertexTopologyID vertex) {
    return getLocalityVertex(vertex).node.id == pando::getCurrentPlace().node.id;
  }

  bool isOwned(VertexTopologyID vertex) {
    return isLocal(vertex);
  }

  void setEdgeData(VertexTopologyID vertex, std::uint64_t off, EdgeData data) {
    setEdgeData(mintEdgeHandle(vertex, off), data);
  }
  pando::GlobalRef<EdgeData> getEdgeData(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeData(mintEdgeHandle(vertex, off));
  }

private:
  pando::Array<Vertex> vertexEdgeOffsets;
  pando::Array<HalfEdge> edgeDestinations;
  pando::Array<VertexData> vertexData;
  pando::Array<EdgeData> edgeData;
  pando::Array<std::uint64_t> topologyToToken;
  galois::HashTable<std::uint64_t, pando::GlobalPtr<Vertex>> tokenToTopology;
};

static_assert(graph_checker<LCSR<std::uint64_t, std::uint64_t>>::value);

} // namespace galois
#endif // PANDO_LIB_GALOIS_GRAPHS_LOCAL_CSR_HPP_
