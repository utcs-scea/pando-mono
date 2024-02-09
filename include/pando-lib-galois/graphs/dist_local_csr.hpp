// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_

#include <pando-rt/export.h>

#include <utility>

#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/local_csr.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

#define FREE 1

namespace galois {

namespace internal {

template <typename VertexType, typename EdgeType>
struct DLCSR_InitializeState {
  using CSR = LCSR<VertexType, EdgeType>;

  DLCSR_InitializeState() = default;
  DLCSR_InitializeState(galois::PerHost<CSR> arrayOfCSRs_,
                        galois::PerThreadVector<VertexType> vertices_,
                        galois::PerThreadVector<EdgeType> edges_,
                        galois::PerThreadVector<uint64_t> edgeCounts_)
      : arrayOfCSRs(arrayOfCSRs_), vertices(vertices_), edges(edges_), edgeCounts(edgeCounts_) {}

  galois::PerHost<CSR> arrayOfCSRs;
  galois::PerThreadVector<VertexType> vertices;
  galois::PerThreadVector<EdgeType> edges;
  galois::PerThreadVector<uint64_t> edgeCounts;
};

} // namespace internal

template <typename VertexType = WMDVertex, typename EdgeType = WMDEdge>
class DistLocalCSR {
public:
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = pando::GlobalPtr<Vertex>;
  using EdgeHandle = pando::GlobalPtr<HalfEdge>;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using EdgeRange = RefSpan<HalfEdge>;
  using EdgeDataRange = pando::Span<EdgeData>;
  using CSR = LCSR<VertexType, EdgeType>;

  class VertexIt {
    galois::PerHost<CSR> arrayOfCSRs{};
    pando::GlobalPtr<Vertex> m_pos;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::int64_t;
    using value_type = typename std::pointer_traits<decltype(&m_pos)>::element_type;
    using pointer = pando::GlobalPtr<Vertex>;
    using reference = VertexTopologyID;

    VertexIt(galois::PerHost<CSR> arrayOfCSRs, VertexTopologyID pos)
        : arrayOfCSRs(arrayOfCSRs), m_pos(pos) {}

    constexpr VertexIt() noexcept = default;
    constexpr VertexIt(VertexIt&&) noexcept = default;
    constexpr VertexIt(const VertexIt&) noexcept = default;
    ~VertexIt() = default;

    constexpr VertexIt& operator=(const VertexIt&) noexcept = default;
    constexpr VertexIt& operator=(VertexIt&&) noexcept = default;

    reference operator*() const noexcept {
      return m_pos;
    }

    reference operator*() noexcept {
      return m_pos;
    }

    pointer operator->() {
      return m_pos;
    }

    VertexIt& operator++() {
      auto currNode = static_cast<std::uint64_t>(galois::localityOf(m_pos).node.id);
      pointer ptr = m_pos + 1;
      CSR csrCurr = arrayOfCSRs.get(currNode);
      if (csrCurr.vertexEdgeOffsets.end() - 1 > ptr ||
          (int16_t)currNode == pando::getPlaceDims().node.id - 1) {
        m_pos = ptr;
      } else {
        csrCurr = arrayOfCSRs.get(currNode + 1);
        this->m_pos = csrCurr.vertexEdgeOffsets.begin();
      }
      return *this;
    }

    VertexIt operator++(int) {
      VertexIt tmp = *this;
      ++(*this);
      return tmp;
    }

    VertexIt& operator--() {
      auto currNode = static_cast<std::uint64_t>(galois::localityOf(m_pos).node.id);
      pointer ptr = m_pos - 1;
      CSR csrCurr = arrayOfCSRs.get(currNode);
      if (csrCurr.vertexEdgeOffsets.begin() <= ptr || currNode == 0) {
        m_pos = ptr;
      } else {
        csrCurr = arrayOfCSRs.get(currNode - 1);
        m_pos = csrCurr.vertexEdgeOffsets.end() - 2;
      }
      return *this;
    }

    VertexIt operator--(int) {
      VertexIt tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const VertexIt& a, const VertexIt& b) {
      return a.m_pos == b.m_pos;
    }

    friend bool operator!=(const VertexIt& a, const VertexIt& b) {
      return !(a == b);
    }

    friend bool operator<(const VertexIt& a, const VertexIt& b) {
      return localityOf(a.m_pos).node.id < localityOf(b.m_pos).node.id || a.m_pos < b.m_pos;
    }

    friend bool operator>(const VertexIt& a, const VertexIt& b) {
      return localityOf(a.m_pos).node.id > localityOf(b.m_pos).node.id || a.m_pos > b.m_pos;
    }

    friend bool operator<=(const VertexIt& a, const VertexIt& b) {
      return !(a > b);
    }

    friend bool operator>=(const VertexIt& a, const VertexIt& b) {
      return !(a < b);
    }

    friend pando::Place localityOf(VertexIt& a) {
      return pando::localityOf(a.m_pos);
    }
  };

  class VertexDataIt {
    pando::Array<CSR> arrayOfCSRs;
    typename CSR::VertexDataRange::iterator m_pos;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::int64_t;
    using value_type = VertexData;
    using pointer = pando::GlobalPtr<VertexData>;
    using reference = pando::GlobalRef<VertexData>;

    VertexDataIt(pando::Array<CSR> arrayOfCSRs, typename pando::GlobalPtr<VertexData> pos)
        : arrayOfCSRs(arrayOfCSRs), m_pos(pos) {}

    constexpr VertexDataIt() noexcept = default;
    constexpr VertexDataIt(VertexDataIt&&) noexcept = default;
    constexpr VertexDataIt(const VertexDataIt&) noexcept = default;
    ~VertexDataIt() = default;

    constexpr VertexDataIt& operator=(const VertexDataIt&) noexcept = default;
    constexpr VertexDataIt& operator=(VertexDataIt&&) noexcept = default;

    reference operator*() const noexcept {
      return *m_pos;
    }

    reference operator*() noexcept {
      return *m_pos;
    }

    pointer operator->() {
      return m_pos;
    }

    VertexDataIt& operator++() {
      auto currNode = static_cast<std::uint64_t>(galois::localityOf(&m_pos).node.id);
      pointer ptr = m_pos + 1;
      CSR csrCurr = arrayOfCSRs[currNode];
      if (csrCurr.vertexData.end() > ptr || currNode == pando::getPlaceDims().node.id - 1) {
        m_pos = ptr;
      } else {
        csrCurr = arrayOfCSRs[currNode + 1];
        m_pos = csrCurr.vertexData.begin();
      }
      return this;
    }

    VertexDataIt operator++(int) {
      VertexIt tmp = *this;
      ++(*this);
      return tmp;
    }

    VertexDataIt& operator--() {
      auto currNode = static_cast<std::uint64_t>(galois::localityOf(&m_pos).node.id);
      pointer ptr = m_pos - 1;
      CSR csrCurr = arrayOfCSRs[currNode];
      if (csrCurr.vertexData.begin() <= ptr || currNode == 0) {
        m_pos = *ptr;
      } else {
        csrCurr = arrayOfCSRs[currNode - 1];
        m_pos = *csrCurr.vertexData.end() - 1;
      }
      return *this;
    }

    VertexIt operator--(int) {
      VertexIt tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const VertexIt& a, const VertexIt& b) {
      return a.m_pos == b.m_pos;
    }

    friend bool operator!=(const VertexIt& a, const VertexIt& b) {
      return !(a == b);
    }

    friend bool operator<(const VertexIt& a, const VertexIt& b) {
      return localityOf(a.m_pos).node.id < localityOf(b.m_pos).node.id || a.m_pos < b.m_pos;
    }

    friend bool operator>(const VertexIt& a, const VertexIt& b) {
      return localityOf(a.m_pos).node.id > localityOf(b.m_pos).node.id || a.m_pos > b.m_pos;
    }

    friend bool operator<=(const VertexIt& a, const VertexIt& b) {
      return !(a > b);
    }

    friend bool operator>=(const VertexIt& a, const VertexIt& b) {
      return !(a < b);
    }

    friend pando::Place localityOf(VertexIt& a) {
      return galois::localityOf(a.m_pos);
    }
  };

  struct VertexRange {
    galois::PerHost<CSR> arrayOfCSRs{};
    VertexTopologyID m_beg;
    VertexTopologyID m_end;
    std::uint64_t m_size;

    constexpr VertexRange() noexcept = default;
    constexpr VertexRange(VertexRange&&) noexcept = default;
    constexpr VertexRange(const VertexRange&) noexcept = default;
    ~VertexRange() = default;

    constexpr VertexRange& operator=(const VertexRange&) noexcept = default;
    constexpr VertexRange& operator=(VertexRange&&) noexcept = default;

    using iterator = VertexIt;
    iterator begin() noexcept {
      return iterator(arrayOfCSRs, m_beg);
    }
    iterator begin() const noexcept {
      return iterator(arrayOfCSRs, m_beg);
    }
    iterator end() noexcept {
      return iterator(arrayOfCSRs, m_end);
    }
    iterator end() const noexcept {
      return iterator(arrayOfCSRs, m_end);
    }
    std::uint64_t size() const {
      return m_size;
    }
  };

  struct VertexDataRange {
    galois::PerHost<CSR> arrayOfCSRs{};
    pando::GlobalPtr<VertexData> m_beg;
    pando::GlobalPtr<VertexData> m_end;
    std::uint64_t m_size;

    using iterator = VertexIt;
    iterator begin() noexcept {
      return iterator(arrayOfCSRs, m_beg);
    }
    iterator begin() const noexcept {
      return iterator(arrayOfCSRs, m_beg);
    }
    iterator end() noexcept {
      return iterator(arrayOfCSRs, m_end);
    }
    iterator end() const noexcept {
      return iterator(arrayOfCSRs, m_end);
    }
    std::uint64_t size() const {
      return m_size;
    }
  };

private:
  template <typename T>
  pando::GlobalRef<CSR> getCSR(pando::GlobalPtr<T> ptr) {
    return arrayOfCSRs.getFromPtr(ptr);
  }

  EdgeHandle halfEdgeBegin(VertexTopologyID vertex) {
    return fmap(getCSR(vertex), halfEdgeBegin, vertex);
  }

  EdgeHandle halfEdgeEnd(VertexTopologyID vertex) {
    return static_cast<Vertex>(*(vertex + 1)).edgeBegin;
  }

  struct InitializeEdgeState {
    InitializeEdgeState() = default;
    InitializeEdgeState(DistLocalCSR<VertexType, EdgeType> dlcsr_,
                        galois::PerThreadVector<EdgeType> edges_,
                        galois::PerThreadVector<VertexTokenID> edgeDsts_)
        : dlcsr(dlcsr_), edges(edges_), edgeDsts(edgeDsts_) {}

    DistLocalCSR<VertexType, EdgeType> dlcsr;
    galois::PerThreadVector<EdgeType> edges;
    galois::PerThreadVector<VertexTokenID> edgeDsts;
  };

  template <typename, typename>
  friend class DistLocalCSR;

public:
  constexpr DistLocalCSR() noexcept = default;
  constexpr DistLocalCSR(DistLocalCSR&&) noexcept = default;
  constexpr DistLocalCSR(const DistLocalCSR&) noexcept = default;
  ~DistLocalCSR() = default;

  constexpr DistLocalCSR& operator=(const DistLocalCSR&) noexcept = default;
  constexpr DistLocalCSR& operator=(DistLocalCSR&&) noexcept = default;

  /** Official Graph APIS **/
  void deinitialize() {
    for (CSR csr : arrayOfCSRs) {
      csr.deinitialize();
    }
    arrayOfCSRs.deinitialize();
    virtualToPhysicalMap.deinitialize();
  }

  /** size stuff **/
  std::uint64_t size() noexcept {
    return numVertices;
  }
  std::uint64_t size() const noexcept {
    return numVertices;
  }
  std::uint64_t sizeEdges() noexcept {
    return numEdges;
  }
  std::uint64_t sizeEdges() const noexcept {
    return numEdges;
  }
  std::uint64_t getNumEdges(VertexTopologyID vertex) {
    return halfEdgeEnd(vertex) - halfEdgeBegin(vertex);
  }

  /** Vertex Manipulation **/
  VertexTopologyID getTopologyID(VertexTokenID tid) {
    std::uint64_t virtualHostID = tid % this->numVHosts;
    std::uint64_t physicalHost = virtualToPhysicalMap[virtualHostID];
    return fmap(arrayOfCSRs.get(physicalHost), getTopologyID, tid);
  }
  VertexTopologyID getTopologyIDFromIndex(std::uint64_t index) {
    std::uint64_t hostNum = 0;
    std::uint64_t hostSize;
    for (; index > (hostSize = localSize(hostNum)); hostNum++, index -= hostSize) {}
    return fmap(arrayOfCSRs.get(hostNum), getTopologyIDFromIndex, index);
  }
  VertexTokenID getTokenID(VertexTopologyID tid) {
    return fmap(getCSR(tid), getTokenID, tid);
  }
  std::uint64_t getVertexIndex(VertexTopologyID vertex) {
    std::uint64_t vid = fmap(getCSR(vertex), getVertexIndex, vertex);
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(getLocalityVertex(vertex).node.id);
         i++) {
      vid += lift(arrayOfCSRs.get(i), size);
    }
    return vid;
  }
  pando::Place getLocalityVertex(VertexTopologyID vertex) {
    // All edges must be local to the vertex
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

  /** Data Manipulations **/
  void setData(VertexTopologyID vertex, VertexData data) {
    fmapVoid(getCSR(vertex), setData, vertex, data);
  }
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return fmap(getCSR(vertex), getData, vertex);
  }
  void setEdgeData(EdgeHandle eh, EdgeData data) {
    fmapVoid(getCSR(eh), setEdgeData, eh, data);
  }
  pando::GlobalRef<EdgeData> getEdgeData(EdgeHandle eh) {
    return fmap(getCSR(eh), getEdgeData, eh);
  }

  /** Ranges **/
  VertexRange vertices() {
    return VertexRange{arrayOfCSRs, lift(arrayOfCSRs.get(0), vertexEdgeOffsets.begin),
                       lift(arrayOfCSRs.get(arrayOfCSRs.size() - 1), vertexEdgeOffsets.end) - 1,
                       numVertices};
  }

  EdgeRange edges(pando::GlobalPtr<galois::Vertex> vPtr) {
    Vertex v = *vPtr;
    Vertex v1 = *(vPtr + 1);
    return RefSpan<galois::HalfEdge>(v.edgeBegin, v1.edgeBegin - v.edgeBegin);
  }
  VertexDataRange vertexDataRange() noexcept {
    return VertexDataRange{arrayOfCSRs, lift(arrayOfCSRs.get(0), vertexData.begin),
                           lift(arrayOfCSRs.get(arrayOfCSRs.size() - 1), vertexData.end),
                           numVertices};
  }
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    return fmap(getCSR(vertex), edgeDataRange, vertex);
  }

  /** Topology Modifications **/
  VertexTopologyID addVertexTopologyOnly(VertexTokenID token) {
    return vertices().end();
  }
  VertexTopologyID addVertex(VertexTokenID token, VertexData data) {
    return vertices().end();
  }
  pando::Status addEdgesTopologyOnly(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts) {
    return addEdges(src, dsts, pando::Vector<EdgeData>());
  }
  pando::Status addEdges(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts,
                         pando::Vector<EdgeData> data) {
    return pando::Status::Error;
  }
  pando::Status deleteEdges(VertexTopologyID src, pando::Vector<EdgeHandle> edges) {
    return pando::Status::Error;
  }

  /**
   * @brief This initializer is used to deal with the outputs of partitioning
   */

  pando::Status initializeAfterGather(
      galois::PerHost<pando::Vector<VertexData>> vertexData, std::uint64_t numVertices,
      galois::PerHost<pando::Vector<pando::Vector<EdgeData>>> edgeData,
      galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> edgeMap,
      pando::Array<std::uint64_t> numEdges, pando::Array<std::uint64_t> virtualToPhysical) {
    this->virtualToPhysicalMap = virtualToPhysical;
    this->numVertices = numVertices;
    this->numVHosts = virtualToPhysical.size();
    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    pando::Status err;
    err = arrayOfCSRs.initialize();
    PANDO_CHECK(err);
    pando::NotificationArray notifier;
    PANDO_CHECK(notifier.init(numHosts));

    galois::PerHost<std::uint64_t> numVerticesPerHost{};
    err = numVerticesPerHost.initialize();
    PANDO_CHECK(err);
    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Vector<VertexType> VertexData = vertexData.get(i);
      numVerticesPerHost.get(i) = lift(vertexData.get(i), size);
    }

    auto createCSRFuncs =
        +[](galois::PerHost<CSR> arrayOfCSRs, galois::PerHost<pando::Vector<VertexData>> vertexData,
            pando::Array<std::uint64_t> numEdges, std::uint64_t i, pando::NotificationHandle done) {
          pando::Status err;
          CSR currentCSR;
          err = currentCSR.initializeTopologyMemory(lift(vertexData.getLocal(), size), numEdges[i]);
          PANDO_CHECK(err);

          err = currentCSR.initializeDataMemory(lift(vertexData.getLocal(), size), numEdges[i]);
          PANDO_CHECK(err);

          std::uint64_t j = 0;
          pando::Vector<VertexType> vertexDataVec = vertexData.getLocal();
          for (VertexData data : vertexDataVec) {
            currentCSR.topologyToToken[j] = data.id;
            currentCSR.vertexData[j] = data;
            err = currentCSR.tokenToTopology.put(data.id, &currentCSR.vertexEdgeOffsets[j]);
            PANDO_CHECK(err);
            j++;
          }
          arrayOfCSRs.getLocal() = currentCSR;
          done.notify();
        };

    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)},
                                        pando::anyPod, pando::anyCore};
      err = pando::executeOn(place, createCSRFuncs, this->arrayOfCSRs, vertexData, numEdges, i,
                             notifier.getHandle(i));
      PANDO_CHECK(err);
    }
    for (std::uint64_t i = 0; i < numEdges.size(); i++) {
      this->numEdges += numEdges[i];
    }
    notifier.wait();
    notifier.reset();

    auto fillCSRFuncs =
        +[](DistLocalCSR<VertexType, EdgeType> dlcsr,
            galois::PerHost<pando::Vector<pando::Vector<EdgeData>>> edgeData,
            galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> edgeMap,
            galois::PerHost<std::uint64_t> numVerticesPerHost, std::uint64_t i,
            pando::NotificationHandle done) {
          CSR currentCSR = dlcsr.arrayOfCSRs.get(i);
          pando::Vector<pando::Vector<EdgeData>> currEdgeData = edgeData.get(i);
          std::uint64_t numVertices = numVerticesPerHost.get(i);
          galois::HashTable<std::uint64_t, std::uint64_t> currEdgeMap = edgeMap.get(i);
          std::uint64_t edgeCurr = 0;
          currentCSR.vertexEdgeOffsets[0] = Vertex{currentCSR.edgeDestinations.begin()};
          for (std::uint64_t vertexCurr = 0; vertexCurr < numVertices; vertexCurr++) {
            std::uint64_t vertexTokenID =
                currentCSR.getTokenID(&currentCSR.vertexEdgeOffsets[vertexCurr]);

            std::uint64_t edgeMapID;
            pando::Vector<EdgeData> edges;
            std::uint64_t size = 0;
            if (currEdgeMap.get(vertexTokenID, edgeMapID)) {
              edges = currEdgeData[edgeMapID];
              size = edges.size();
            }

            for (std::uint64_t j = 0; j < size; j++, edgeCurr++) {
              HalfEdge e;
              EdgeType eData = edges[j];
              e.dst = dlcsr.getTopologyID(eData.dst);
              currentCSR.edgeDestinations[edgeCurr] = e;
              pando::GlobalPtr<HalfEdge> eh = &currentCSR.edgeDestinations[edgeCurr];
              currentCSR.setEdgeData(eh, edges[j]);
            }
            currentCSR.vertexEdgeOffsets[vertexCurr + 1] =
                Vertex{&currentCSR.edgeDestinations[edgeCurr]};
          }
          dlcsr.arrayOfCSRs.getLocal() = currentCSR;
          done.notify();
        };
    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)},
                                        pando::anyPod, pando::anyCore};
      err = pando::executeOn(place, fillCSRFuncs, *this, edgeData, edgeMap, numVerticesPerHost, i,
                             notifier.getHandle(i));
      PANDO_CHECK(err);
    }
    notifier.wait();

    return pando::Status::Success;
  }

  pando::Status initializeAfterImport(galois::PerThreadVector<VertexType>&& localVertices,
                                      galois::PerThreadVector<pando::Vector<EdgeType>>&& localEdges,
                                      uint64_t numVerticesRead, bool isEdgelist) {
    pando::Status err;
    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    std::uint16_t scale_factor = 8;
    std::uint64_t numVHosts = numHosts * scale_factor;

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts;
    pando::LocalStorageGuard labeledEdgeCountsGuard(labeledEdgeCounts, 1);
    galois::internal::buildEdgeCountToSend<EdgeType>(numVHosts, localEdges, *labeledEdgeCounts);

    pando::GlobalPtr<pando::Array<std::uint64_t>> v2PM;
    pando::LocalStorageGuard vTPMGuard(v2PM, 1);

    pando::Array<std::uint64_t> numEdges;
    err = numEdges.initialize(hosts);

    err =
        galois::internal::buildVirtualToPhysicalMapping(hosts, *labeledEdgeCounts, v2PM, numEdges);
    if (err != pando::Status::Success) {
      return err;
    }

#if FREE
    auto freeLabeledEdgeCounts =
        +[](pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledEdgeCounts) {
          labeledEdgeCounts.deinitialize();
        };
    PANDO_CHECK_RETURN(pando::executeOn(
        pando::anyPlace, freeLabeledEdgeCounts,
        static_cast<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>>(*labeledEdgeCounts)));
#endif

    pando::GlobalPtr<galois::PerHost<pando::Vector<VertexType>>> readPart{};
    pando::LocalStorageGuard readPartGuard(readPart, 1);
    err = localVertices.hostFlatten((*readPart));

#if FREE
    auto freeLocalVertices = +[](PerThreadVector<VertexType> localVertices) {
      localVertices.deinitialize();
    };
    PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freeLocalVertices, localVertices));
#endif

    galois::PerHost<pando::Vector<VertexType>> pHV{};
    err = pHV.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    if (!isEdgelist) {
      galois::PerHost<galois::PerHost<pando::Vector<VertexType>>> partVert{};
      err = partVert.initialize();
      if (err != pando::Status::Success) {
        return err;
      }

      struct PHPV {
        pando::Array<std::uint64_t> v2PM;
        PerHost<pando::Vector<VertexType>> pHV;
      };

      auto f = +[](PHPV phpv, pando::GlobalRef<PerHost<pando::Vector<VertexType>>> partVert) {
        const std::uint64_t hostID = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
        PANDO_CHECK(galois::internal::perHostPartitionVertex<VertexType>(
            phpv.v2PM, phpv.pHV.get(hostID), &partVert));
      };
      err = galois::doAll(PHPV{*v2PM, *readPart}, partVert, f);
      if (err != pando::Status::Success) {
        return err;
      }

#if FREE
      auto freeReadPart = +[](PerHost<pando::Vector<VertexType>> readPart) {
        for (pando::Vector<VertexType> vec : readPart) {
          vec.deinitialize();
        }
        readPart.deinitialize();
      };
      PANDO_CHECK_RETURN(
          pando::executeOn(pando::anyPlace, freeReadPart,
                           static_cast<PerHost<pando::Vector<VertexType>>>(*readPart)));
#endif

      PANDO_CHECK_RETURN(galois::doAll(
          partVert, pHV,
          +[](decltype(partVert) pHPHV, pando::GlobalRef<pando::Vector<VertexType>> pHV) {
            PANDO_CHECK(fmap(pHV, initialize, 0));
            std::uint64_t currNode = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
            for (galois::PerHost<pando::Vector<VertexType>> pHVRef : pHPHV) {
              PANDO_CHECK(fmap(pHV, append, &pHVRef.get(currNode)));
            }
          }));
#if FREE
      auto freePartVert = +[](PerHost<PerHost<pando::Vector<VertexType>>> partVert) {
        for (PerHost<pando::Vector<VertexType>> pVV : partVert) {
          for (pando::Vector<VertexType> vV : pVV) {
            vV.deinitialize();
          }
          pVV.deinitialize();
        }
        partVert.deinitialize();
      };
      PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freePartVert, partVert));
#endif
    }

    galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partEdges{};
    err = partEdges.initialize();
    if (err != pando::Status::Success) {
      return err;
    }
    for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vvec : partEdges) {
      PANDO_CHECK(fmap(vvec, initialize, 0));
    }

    galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
    err = renamePerHost.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    err = galois::internal::partitionEdgesSerially(localEdges, *v2PM, partEdges, renamePerHost);
    if (err != pando::Status::Success) {
      return err;
    }

#if FREE
    auto freeLocalEdges = +[](PerThreadVector<pando::Vector<EdgeType>> localEdges) {
      for (pando::Vector<pando::Vector<EdgeType>> vVE : localEdges) {
        for (pando::Vector<EdgeType> vE : vVE) {
          vE.deinitialize();
        }
      }
      localEdges.deinitialize();
    };
    PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freeLocalEdges, localEdges));
#endif

    if (isEdgelist) {
      for (uint64_t h = 0; h < numHosts; h++) {
        PANDO_CHECK(fmap(pHV.get(h), initialize, 0));
      }
      struct PHPV {
        PerHost<pando::Vector<pando::Vector<EdgeType>>> partEdges;
        PerHost<pando::Vector<VertexType>> pHV;
      };
      PHPV phpv{partEdges, pHV};
      galois::doAllEvenlyPartition(
          phpv, numHosts, +[](PHPV phpv, uint64_t host_id, uint64_t) {
            pando::Vector<pando::Vector<EdgeType>> edgeVec = phpv.partEdges.get(host_id);
            pando::GlobalRef<pando::Vector<VertexType>> vertexVec = phpv.pHV.get(host_id);
            for (pando::Vector<EdgeType> vec : edgeVec) {
              EdgeType e = vec[0];
              VertexType v = VertexType(e.src, agile::TYPES::NONE);
              PANDO_CHECK(fmap(vertexVec, pushBack, v));
            }
          });
    }
    uint64_t numVerticesInEdgelist = 0;
    for (uint64_t h = 0; h < numHosts; h++) {
      numVerticesInEdgelist += lift(pHV.get(h), size);
    }

    uint64_t numVertices = isEdgelist ? numVerticesInEdgelist : numVerticesRead;

    err = initializeAfterGather(pHV, numVertices, partEdges, renamePerHost, numEdges, *v2PM);
#if FREE
    auto freeTheRest = +[](decltype(pHV) pHV, decltype(partEdges) partEdges,
                           decltype(renamePerHost) renamePerHost, decltype(numEdges) numEdges) {
      for (pando::Vector<VertexType> vV : pHV) {
        vV.deinitialize();
      }
      pHV.deinitialize();
      for (pando::Vector<pando::Vector<EdgeType>> vVE : partEdges) {
        for (pando::Vector<EdgeType> vE : vVE) {
          vE.deinitialize();
        }
        vVE.deinitialize();
      }
      partEdges.deinitialize();
      renamePerHost.deinitialize();
      numEdges.deinitialize();
    };

    PANDO_CHECK_RETURN(
        pando::executeOn(pando::anyPlace, freeTheRest, pHV, partEdges, renamePerHost, numEdges));
#endif

    return err;
  }

  /**
   * @brief This initializer just passes in a file and constructs a WMD graph.
   */
  pando::Status initializeWMD(pando::Array<char> filename, bool isEdgelist) {
    pando::Status err;

    std::uint64_t numThreads = 32;
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges;
    err = localEdges.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    pando::Array<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename;
    err = perThreadRename.initialize(localEdges.size());
    if (err != pando::Status::Success) {
      return err;
    }
    perThreadRename.fill(galois::HashTable<std::uint64_t, std::uint64_t>{});
    for (auto hashRef : perThreadRename) {
      err = fmap(hashRef, initialize, 0);
      if (err != pando::Status::Success) {
        return err;
      }
    }

    galois::PerThreadVector<VertexType> localVertices;
    err = localVertices.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    pando::NotificationArray dones;
    err = dones.init(numThreads);
    if (err != pando::Status::Success) {
      return err;
    }
    galois::DAccumulator<std::uint64_t> totVerts;
    err = totVerts.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    for (std::uint64_t i = 0; i < numThreads; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i % hosts)},
                                        pando::anyPod, pando::anyCore};
      err = pando::executeOn(place, &galois::internal::loadGraphFilePerThread<VertexType, EdgeType>,
                             dones.getHandle(i), filename, 1, numThreads, i, isEdgelist, localEdges,
                             perThreadRename, localVertices, totVerts);
      if (err != pando::Status::Success) {
        return err;
      }
    }
    dones.wait();

#if FREE
    auto freePerThreadRename =
        +[](pando::Array<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename) {
          for (galois::HashTable<std::uint64_t, std::uint64_t> hash : perThreadRename) {
            hash.deinitialize();
          }
          perThreadRename.deinitialize();
        };
    PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freePerThreadRename, perThreadRename));
#endif

    err = initializeAfterImport(std::move(localVertices), std::move(localEdges), totVerts.reduce(),
                                isEdgelist);
    totVerts.deinitialize();
    return err;
  }

  pando::Status initialize(pando::Vector<galois::VertexParser<VertexType>>&& vertexParsers,
                           pando::Vector<galois::EdgeParser<EdgeType>>&& edgeParsers) {
    std::uint64_t numThreads = 32;
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges;
    PANDO_CHECK_RETURN(localEdges.initialize());

    pando::Array<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename;
    PANDO_CHECK_RETURN(perThreadRename.initialize(localEdges.size()));
    perThreadRename.fill(galois::HashTable<std::uint64_t, std::uint64_t>{});
    for (auto hashRef : perThreadRename) {
      PANDO_CHECK_RETURN(fmap(hashRef, initialize, 0));
    }

    galois::PerThreadVector<VertexType> localVertices;
    PANDO_CHECK_RETURN(localVertices.initialize());

    for (VertexParser<VertexType> vertexParser : vertexParsers) {
      pando::NotificationArray dones;
      PANDO_CHECK_RETURN(dones.init(numThreads));

      std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
      for (std::uint64_t i = 0; i < numThreads; i++) {
        pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i % hosts)},
                                          pando::anyPod, pando::anyCore};
        PANDO_CHECK_RETURN(
            pando::executeOn(place, &galois::internal::loadVertexFilePerThread<VertexType>,
                             dones.getHandle(i), vertexParser, 1, numThreads, i, localVertices));
      }
      dones.wait();
    }
    for (galois::EdgeParser<EdgeType> edgeParser : edgeParsers) {
      pando::NotificationArray dones;
      PANDO_CHECK_RETURN(dones.init(numThreads));

      std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
      for (std::uint64_t i = 0; i < numThreads; i++) {
        pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i % hosts)},
                                          pando::anyPod, pando::anyCore};
        PANDO_CHECK_RETURN(pando::executeOn(
            place, &galois::internal::loadEdgeFilePerThread<EdgeType>, dones.getHandle(i),
            edgeParser, 1, numThreads, i, localEdges, perThreadRename));
      }
      dones.wait();
    }

    vertexParsers.deinitialize();
    edgeParsers.deinitialize();

#if FREE
    auto freePerThreadRename =
        +[](pando::Array<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename) {
          for (galois::HashTable<std::uint64_t, std::uint64_t> hash : perThreadRename) {
            hash.deinitialize();
          }
          perThreadRename.deinitialize();
        };
    PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freePerThreadRename, perThreadRename));
#endif

    const bool isEdgeList = false;
    PANDO_CHECK_RETURN(initializeAfterImport(std::move(localVertices), std::move(localEdges),
                                             localVertices.sizeAll(), isEdgeList));
    return pando::Status::Success;
  }

  /**
   * @brief This initializer for workflow 4's edge lists
   */
  pando::Status initializeWMD(pando::Vector<galois::EdgeParser<EdgeType>>&& edgeParsers,
                              const uint64_t chunk_size = 10000, const uint64_t scale_factor = 8) {
    pando::Status err;

    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    std::uint64_t numVHosts = numHosts * scale_factor;
    galois::PerThreadVector<EdgeType> localEdges;
    err = localEdges.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    for (galois::EdgeParser<EdgeType> parser : edgeParsers) {
      galois::ifstream graphFile;
      PANDO_CHECK(graphFile.open(parser.filename));
      uint64_t fileSize = graphFile.size();
      uint64_t segments = (fileSize / chunk_size) + 1;
      graphFile.close();
      galois::doAllEvenlyPartition(internal::ImportState<EdgeType>(parser, localEdges), segments,
                                   &internal::loadGraphFile<EdgeType>);
    }
    edgeParsers.deinitialize();

    pando::GlobalPtr<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts;
    pando::LocalStorageGuard labeledEdgeCountsGuard(labeledEdgeCounts, 1);
    galois::internal::buildEdgeCountToSend<EdgeType>(numVHosts, localEdges, *labeledEdgeCounts);

    pando::GlobalPtr<pando::Array<std::uint64_t>> v2PM;
    pando::LocalStorageGuard vTPMGuard(v2PM, 1);

    pando::Array<std::uint64_t> numEdges;
    err = numEdges.initialize(numHosts);

    err = galois::internal::buildVirtualToPhysicalMapping(numHosts, *labeledEdgeCounts, v2PM,
                                                          numEdges);
    if (err != pando::Status::Success) {
      return err;
    }

    galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partEdges{};
    err = partEdges.initialize();
    if (err != pando::Status::Success) {
      return err;
    }
    for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vvec : partEdges) {
      PANDO_CHECK(fmap(vvec, initialize, 0));
    }

    galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
    err = renamePerHost.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    err = galois::internal::partitionEdgesSerially<EdgeType>(localEdges, *v2PM, partEdges,
                                                             renamePerHost);
    if (err != pando::Status::Success) {
      return err;
    }

    galois::PerHost<pando::Vector<VertexType>> pHV{};
    err = pHV.initialize();
    if (err != pando::Status::Success) {
      return err;
    }

    PANDO_CHECK_RETURN(galois::doAll(
        partEdges, pHV,
        +[](galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partEdges,
            pando::GlobalRef<pando::Vector<VertexType>> pHV) {
          PANDO_CHECK(fmap(pHV, initialize, 0));
          pando::Vector<pando::Vector<EdgeType>> localEdges = partEdges.getLocal();
          for (pando::Vector<EdgeType> e : localEdges) {
            EdgeType e0 = e[0];
            VertexType v0 = VertexType(e0.src, e0.srcType);
            PANDO_CHECK(fmap(pHV, pushBack, v0));
          }
        }));
    uint64_t srcVertices = 0;
    for (pando::Vector<VertexType> hostVertices : pHV) {
      srcVertices += hostVertices.size();
    }

    err = initializeAfterGather(pHV, srcVertices, partEdges, renamePerHost, numEdges, *v2PM);
    return err;
  }

  /**
   * @brief Creates a DistLocalCSR from an explicit graph definition, intended only for tests
   *
   * @param[in] vertices This is a vector of vertex values.
   * @param[in] edges This is vector global src id, dst id, and edge data.
   */
  [[nodiscard]] pando::Status initialize(pando::Vector<VertexType> vertices,
                                         pando::Vector<GenericEdge<EdgeType>> edges) {
    numVertices = vertices.size();
    numEdges = edges.size();
    numVHosts = vertices.size();
    PANDO_CHECK_RETURN(virtualToPhysicalMap.initialize(numVHosts));
    std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    uint64_t verticesPerHost = numVertices / hosts;
    if (hosts * verticesPerHost < numVertices) {
      verticesPerHost++;
    }
    pando::Vector<uint64_t> edgeCounts;
    PANDO_CHECK_RETURN(edgeCounts.initialize(verticesPerHost));
    uint64_t edgesStart = 0;
    for (uint64_t host = 0; host < hosts; host++) {
      uint64_t vertex;
      uint64_t edgesEnd = edgesStart;
      for (vertex = 0; vertex < verticesPerHost && vertex + host * verticesPerHost < numVertices;
           vertex++) {
        uint64_t currLocalVertex = vertex + host * verticesPerHost;
        virtualToPhysicalMap[currLocalVertex] = host;
        uint64_t vertexEdgeStart = edgesEnd;
        for (; edgesEnd < numEdges && (&edges[edgesEnd])->src <= (&vertices[currLocalVertex])->id;
             edgesEnd++) {}
        edgeCounts[vertex] = edgesEnd - vertexEdgeStart;
      }
      CSR currentCSR{};
      uint64_t numLocalEdges = edgesEnd - edgesStart;
      PANDO_CHECK_RETURN(currentCSR.initializeTopologyMemory(
          vertex, numLocalEdges,
          pando::Place{pando::NodeIndex{(int16_t)host}, pando::anyPod, pando::anyCore},
          pando::MemoryType::Main));
      PANDO_CHECK_RETURN(currentCSR.initializeDataMemory(
          vertex, numLocalEdges,
          pando::Place{pando::NodeIndex{(int16_t)host}, pando::anyPod, pando::anyCore},
          pando::MemoryType::Main));

      uint64_t currLocalEdge = 0;
      for (uint64_t v = 0; v < vertex; v++) {
        uint64_t currLocalVertex = v + host * verticesPerHost;
        VertexData data = vertices[currLocalVertex];
        currentCSR.topologyToToken[v] = data.id;
        currentCSR.vertexData[v] = data;
        currentCSR.vertexEdgeOffsets[v] = Vertex{&currentCSR.edgeDestinations[currLocalEdge]};
        PANDO_CHECK_RETURN(
            currentCSR.tokenToTopology.put(data.id, &currentCSR.vertexEdgeOffsets[v]));
        currLocalEdge += edgeCounts[v];
      }
      currentCSR.vertexEdgeOffsets[vertex] = Vertex{&currentCSR.edgeDestinations[currLocalEdge]};

      arrayOfCSRs.get(host) = currentCSR;
      edgesStart = edgesEnd;
    }
    edgeCounts.deinitialize();

    edgesStart = 0;
    for (uint64_t host = 0; host < hosts; host++) {
      CSR currentCSR = arrayOfCSRs.get(host);

      uint64_t lastLocalVertexIndex = verticesPerHost * (host + 1) - 1;
      if (lastLocalVertexIndex >= numVertices) {
        lastLocalVertexIndex = numVertices - 1;
      }
      uint64_t lastLocalVertex = (&vertices[lastLocalVertexIndex])->id;

      uint64_t currLocalEdge = 0;
      GenericEdge<EdgeType> currEdge = edges[edgesStart];
      for (; edgesStart + currLocalEdge < numEdges && currEdge.src <= lastLocalVertex;
           currLocalEdge++) {
        EdgeType data = currEdge.data;
        HalfEdge edge;
        edge.dst = getTopologyID(currEdge.dst);
        currentCSR.edgeDestinations[currLocalEdge] = edge;
        currentCSR.setEdgeData(&currentCSR.edgeDestinations[currLocalEdge], data);

        if (currLocalEdge + edgesStart < numEdges - 1) {
          currEdge = edges[edgesStart + currLocalEdge + 1];
        }
      }
      arrayOfCSRs.get(host) = currentCSR;

      edgesStart += currLocalEdge;
    }
    return pando::Status::Success;
  }

  /**
   * @brief Creates a DistArrayCSR from an explicit graph definition, intended only for tests
   *
   * @param[in] vertices This is a vector of vertex values with TokenIDs in an `id` field.
   * @param[in] edges This is a vector of edge data.
   * @param[in] edgeDsts This is a vector of token dst id
   * @param[in] edgeCounts This is a vector of edge counts
   * @warning Edges must be ordered by vertex, but vertex IDs may not contiguous
   */
  template <typename OldGraph>
  [[nodiscard]] pando::Status initialize(OldGraph& oldGraph,
                                         galois::PerThreadVector<VertexType> vertices,
                                         galois::PerThreadVector<EdgeType> edges,
                                         galois::PerThreadVector<VertexTokenID> edgeDsts,
                                         galois::PerThreadVector<uint64_t> edgeCounts) {
    numVertices = vertices.sizeAll();
    numEdges = edges.sizeAll();
    numVHosts = oldGraph.numVHosts;
    PANDO_CHECK_RETURN(virtualToPhysicalMap.initialize(oldGraph.virtualToPhysicalMap.size()));
    for (uint64_t i = 0; i < virtualToPhysicalMap.size(); i++) {
      virtualToPhysicalMap[i] = oldGraph.virtualToPhysicalMap[i];
    }

    std::uint64_t hosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    PANDO_CHECK_RETURN(arrayOfCSRs.initialize());
    PANDO_CHECK_RETURN(vertices.computeIndices());
    PANDO_CHECK_RETURN(edges.computeIndices());
    PANDO_CHECK_RETURN(edgeDsts.computeIndices());
    internal::DLCSR_InitializeState<VertexType, EdgeType> state(arrayOfCSRs, vertices, edges,
                                                                edgeCounts);
    galois::doAllEvenlyPartition(
        state, hosts,
        +[](internal::DLCSR_InitializeState<VertexType, EdgeType>& state, std::uint64_t host,
            uint64_t hosts) {
          CSR currentCSR{};
          uint64_t numLocalVertices;
          uint64_t numLocalEdges;
          PANDO_CHECK(state.vertices.localElements(numLocalVertices));
          PANDO_CHECK(state.edges.localElements(numLocalEdges));
          PANDO_CHECK(currentCSR.initializeTopologyMemory(numLocalVertices, numLocalEdges));
          PANDO_CHECK(currentCSR.initializeDataMemory(numLocalVertices, numLocalEdges));

          uint64_t currLocalVertex = 0;
          uint64_t currLocalEdge = 0;
          const uint64_t numLocalVectors = state.vertices.size() / hosts;
          for (uint64_t i = host * numLocalVectors; i < (host + 1) * numLocalVectors; i++) {
            pando::Vector<VertexData> vertexData = state.vertices[i];
            pando::Vector<uint64_t> edgeCounts = state.edgeCounts[i];
            for (uint64_t j = 0; j < vertexData.size(); j++) {
              VertexData data = vertexData[j];
              currentCSR.topologyToToken[currLocalVertex] = data.id;
              currentCSR.vertexData[currLocalVertex] = data;
              currentCSR.vertexEdgeOffsets[currLocalVertex] =
                  Vertex{&currentCSR.edgeDestinations[currLocalEdge]};
              PANDO_CHECK(currentCSR.tokenToTopology.put(
                  data.id, &currentCSR.vertexEdgeOffsets[currLocalVertex]));
              currLocalVertex++;
              currLocalEdge += edgeCounts[j];
            }
          }
          currentCSR.vertexEdgeOffsets[currLocalVertex] =
              Vertex{&currentCSR.edgeDestinations[currLocalEdge]};
          state.arrayOfCSRs.getLocal() = currentCSR;
        });
    arrayOfCSRs = state.arrayOfCSRs;

    InitializeEdgeState state2(*this, edges, edgeDsts);
    galois::onEach(
        state2, +[](InitializeEdgeState& state, uint64_t thread, uint64_t) {
          uint64_t host = static_cast<std::uint64_t>(pando::getCurrentNode().id);
          CSR currentCSR = state.dlcsr.arrayOfCSRs.get(host);

          uint64_t hostOffset;
          PANDO_CHECK(state.edges.currentHostIndexOffset(hostOffset));
          uint64_t threadOffset;
          PANDO_CHECK(state.edges.indexOnThread(thread, threadOffset));
          threadOffset -= hostOffset;

          uint64_t currLocalEdge = 0;
          pando::Vector<EdgeData> edgeData = state.edges[thread];
          pando::Vector<VertexTokenID> edgeDsts = state.edgeDsts[thread];
          for (EdgeData data : edgeData) {
            HalfEdge edge;
            edge.dst = state.dlcsr.getTopologyID(edgeDsts[currLocalEdge]);
            currentCSR.edgeDestinations[threadOffset + currLocalEdge] = edge;
            currentCSR.setEdgeData(&currentCSR.edgeDestinations[threadOffset + currLocalEdge],
                                   data);
            currLocalEdge++;
          }
          state.dlcsr.arrayOfCSRs.getLocal() = currentCSR;
        });
    *this = state2.dlcsr;

    return pando::Status::Success;
  }

  /**
   * @brief get vertex local dense ID
   */
  std::uint64_t getVertexLocalIndex(VertexTopologyID vertex) {
    std::uint64_t hostNum = static_cast<std::uint64_t>(galois::localityOf(vertex).node.id);
    return fmap(arrayOfCSRs.get(hostNum), getVertexIndex, vertex);
  }

  /**
   * @brief gives the number of edges
   */

  std::uint64_t localSize(std::uint32_t host) noexcept {
    return lift(arrayOfCSRs.get(host), size);
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
  pando::GlobalRef<EdgeData> getEdgeData(VertexTopologyID vertex, std::uint64_t off) {
    return getEdgeData(mintEdgeHandle(vertex, off));
  }

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

  /**
   * @brief Get the local csr
   */
  pando::GlobalRef<CSR> getLocalCSR() {
    std::uint64_t nodeIdx = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
    return arrayOfCSRs.get(nodeIdx);
  }

private:
  galois::PerHost<CSR> arrayOfCSRs;
  std::uint64_t numVertices;
  std::uint64_t numEdges;
  std::uint64_t numVHosts;
  pando::Array<std::uint64_t> virtualToPhysicalMap;
};

static_assert(graph_checker<DistLocalCSR<std::uint64_t, std::uint64_t>>::value);
static_assert(graph_checker<DistLocalCSR<WMDVertex, WMDEdge>>::value);

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_
