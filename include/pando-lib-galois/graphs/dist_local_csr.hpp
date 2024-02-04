// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_

#include <pando-rt/export.h>

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

template <typename VertexType = WMDVertex, typename EdgeType = WMDEdge>
class DistLocalCSR {
public:
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = pando::GlobalRef<Vertex>;
  using EdgeHandle = pando::GlobalRef<HalfEdge>;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using EdgeRange = pando::Span<HalfEdge>;
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
        : arrayOfCSRs(arrayOfCSRs), m_pos(&pos) {}

    constexpr VertexIt() noexcept = default;
    constexpr VertexIt(VertexIt&&) noexcept = default;
    constexpr VertexIt(const VertexIt&) noexcept = default;
    ~VertexIt() = default;

    constexpr VertexIt& operator=(const VertexIt&) noexcept = default;
    constexpr VertexIt& operator=(VertexIt&&) noexcept = default;

    reference operator*() const noexcept {
      return *m_pos;
    }

    reference operator*() noexcept {
      return *m_pos;
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

  constexpr DistLocalCSR() noexcept = default;
  constexpr DistLocalCSR(DistLocalCSR&&) noexcept = default;
  constexpr DistLocalCSR(const DistLocalCSR&) noexcept = default;
  ~DistLocalCSR() = default;

  constexpr DistLocalCSR& operator=(const DistLocalCSR&) noexcept = default;
  constexpr DistLocalCSR& operator=(DistLocalCSR&&) noexcept = default;

  EdgeRange edges(pando::GlobalRef<galois::Vertex> vertex) {
    pando::GlobalPtr<Vertex> vPtr = &vertex;
    Vertex v = *vPtr;
    Vertex v1 = *(vPtr + 1);
    return pando::Span<galois::HalfEdge>(v.edgeBegin, v1.edgeBegin - v.edgeBegin);
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
                currentCSR.getTokenID(currentCSR.vertexEdgeOffsets[vertexCurr]);

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
              e.dst = &dlcsr.getTopologyID(eData.dst);
              currentCSR.edgeDestinations[edgeCurr] = e;
              pando::GlobalRef<HalfEdge> eh = currentCSR.edgeDestinations[edgeCurr];
              currentCSR.setEdgeData(currentCSR.edgeDestinations[edgeCurr], edges[j]);
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

  /**
   * @brief This initializer just passes in a file and constructs a WMD graph.
   */
  pando::Status initializeWMD(pando::Array<char> filename, bool isEdgelist) {
    pando::Status err;

    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    std::uint16_t scale_factor = 8;
    std::uint64_t numVHosts = numHosts * scale_factor;
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
      PANDO_CHECK_RETURN(galois::doAll(
          partEdges, pHV,
          +[](decltype(partEdges) partEdges, pando::GlobalRef<pando::Vector<VertexType>> pHV) {
            PANDO_CHECK(fmap(pHV, initialize, 0));
            for (pando::Vector<pando::Vector<EdgeType>> e1 : partEdges) {
              for (pando::Vector<EdgeType> e : e1) {
                EdgeType e0 = e[0];
                VertexType v0 = VertexType(e0.src, agile::TYPES::NONE);
                PANDO_CHECK(fmap(pHV, pushBack, v0));
              }
            }
          }));
    }

    err = initializeAfterGather(pHV, totVerts.reduce(), partEdges, renamePerHost, numEdges, *v2PM);
#if FREE
    auto freeTheRest =
        +[](decltype(pHV) pHV, decltype(totVerts) totVerts, decltype(partEdges) partEdges,
            decltype(renamePerHost) renamePerHost, decltype(numEdges) numEdges) {
          for (pando::Vector<VertexType> vV : pHV) {
            vV.deinitialize();
          }
          pHV.deinitialize();
          totVerts.deinitialize();
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

    PANDO_CHECK_RETURN(pando::executeOn(pando::anyPlace, freeTheRest, pHV, totVerts, partEdges,
                                        renamePerHost, numEdges));
#endif

    return err;
  }

  /**
   * @brief Frees all memory and objects associated with this structure
   */
  void deinitialize() {
    for (CSR csr : arrayOfCSRs) {
      csr.deinitialize();
    }
    arrayOfCSRs.deinitialize();
    virtualToPhysicalMap.deinitialize();
  }

private:
  template <typename T>
  pando::GlobalRef<CSR> getCSR(pando::GlobalRef<T> ref) {
    return arrayOfCSRs.getFromPtr(&ref);
  }

  EdgeHandle halfEdgeBegin(VertexTopologyID vertex) {
    return fmap(getCSR(vertex), halfEdgeBegin, vertex);
  }

  EdgeHandle halfEdgeEnd(VertexTopologyID vertex) {
    return *static_cast<Vertex>(*(&vertex + 1)).edgeBegin;
  }

public:
  /**
   * @brief get topology ID from Token ID
   */
  VertexTopologyID getTopologyID(VertexTokenID tid) {
    std::uint64_t virtualHostID = tid % this->numVHosts;
    std::uint64_t physicalHost = virtualToPhysicalMap[virtualHostID];
    return fmap(arrayOfCSRs.get(physicalHost), getTopologyID, tid);
  }

  /**
   * @brief get tokenId from TopologyID
   */
  VertexTokenID getTokenID(VertexTopologyID tid) {
    std::uint64_t hostNum = static_cast<std::uint64_t>(galois::localityOf(&tid).node.id);
    return fmap(arrayOfCSRs.get(hostNum), getTokenID, tid);
  }

  std::uint64_t getVertexIndex(VertexTopologyID vertex) {
    std::uint64_t vid = fmap(getCSR(vertex), getVertexIndex, vertex);
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(getLocalityVertex(vertex).node.id);
         i++) {
      vid += lift(arrayOfCSRs.get(i), size);
    }
    return vid;
  }

  /**
   * @brief get vertex local dense ID
   */
  std::uint64_t getVertexLocalIndex(VertexTopologyID vertex) {
    std::uint64_t hostNum = static_cast<std::uint64_t>(galois::localityOf(&vertex).node.id);
    return fmap(arrayOfCSRs.get(hostNum), getVertexIndex, vertex);
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

  std::uint64_t localSize(std::uint32_t host) noexcept {
    return lift(arrayOfCSRs.get(host), size);
  }

  /**
   * @brief Sets the value of the vertex provided
   */
  void setData(VertexTopologyID vertex, VertexData data) {
    fmapVoid(getCSR(vertex), setData, vertex, data);
  }

  /**
   * @brief gets the value of the vertex provided
   */
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return fmap(getCSR(vertex), getData, vertex);
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
    fmapVoid(getCSR(eh), setEdgeData, eh, data);
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
    return fmap(getCSR(eh), getEdgeData, eh);
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
    return &halfEdgeEnd(vertex) - &halfEdgeBegin(vertex);
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
    // All edges must be local to the vertex
    return galois::localityOf(&vertex);
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
    return VertexRange{arrayOfCSRs, *lift(arrayOfCSRs.get(0), vertexEdgeOffsets.begin),
                       *(lift(arrayOfCSRs.get(arrayOfCSRs.size() - 1), vertexEdgeOffsets.end) - 1),
                       numVertices};
  }

  /**:
   * @brief Get the VertexDataRange for the graph
   */
  VertexDataRange vertexDataRange() noexcept {
    return VertexDataRange{arrayOfCSRs, lift(arrayOfCSRs.get(0), vertexData.begin),
                           lift(arrayOfCSRs.get(arrayOfCSRs.size() - 1), vertexData.end),
                           numVertices};
  }

  /**
   * @brief Get the EdgeDataRange for the graph
   */
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    return fmap(getCSR(vertex), edgeDataRange, vertex);
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
  std::uint64_t numVHosts;
  pando::Array<std::uint64_t> virtualToPhysicalMap;
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_DIST_LOCAL_CSR_HPP_
