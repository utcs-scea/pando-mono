// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_MIRROR_DIST_LOCAL_CSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_MIRROR_DIST_LOCAL_CSR_HPP_

#include <pando-rt/export.h>

#include <utility>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/host_indexed_map.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/local_csr.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/global_barrier.hpp>
#include <pando-lib-galois/sync/simple_lock.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

#define FREE 1

namespace galois {

namespace internal {

template <typename VertexType, typename EdgeType>
struct MDLCSR_InitializeState {
  using CSR = LCSR<VertexType, EdgeType>;

  MDLCSR_InitializeState() = default;
  MDLCSR_InitializeState(galois::HostIndexedMap<CSR> arrayOfCSRs_,
                         galois::PerThreadVector<VertexType> vertices_,
                         galois::PerThreadVector<EdgeType> edges_,
                         galois::PerThreadVector<uint64_t> edgeCounts_)
      : arrayOfCSRs(arrayOfCSRs_), vertices(vertices_), edges(edges_), edgeCounts(edgeCounts_) {}

  galois::HostIndexedMap<CSR> arrayOfCSRs;
  galois::PerThreadVector<VertexType> vertices;
  galois::PerThreadVector<EdgeType> edges;
  galois::PerThreadVector<uint64_t> edgeCounts;
};

} // namespace internal

template <typename VertexType = WMDVertex, typename EdgeType = WMDEdge>
class MirrorDistLocalCSR {
public:
  using VertexTokenID = std::uint64_t;
  using VertexTopologyID = pando::GlobalPtr<Vertex>;
  using EdgeHandle = pando::GlobalPtr<HalfEdge>;
  using VertexData = VertexType;
  using EdgeData = EdgeType;
  using EdgeRange = RefSpan<HalfEdge>;
  using EdgeDataRange = pando::Span<EdgeData>;
  using CSR = LCSR<VertexType, EdgeType>;
  using DLCSR = DistLocalCSR<VertexType, EdgeType>;
  using VertexRange = typename DLCSR::VertexRange;
  using VertexDataRange = typename DLCSR::VertexDataRange;
  using LocalVertexRange = typename CSR::VertexRange;
  using LocalVertexDataRange = typename CSR::VertexDataRange;

private:
  template <typename T>
  pando::GlobalRef<CSR> getCSR(pando::GlobalPtr<T> ptr) {
    return dlcsr.getCSR(ptr);
  }

  EdgeHandle halfEdgeBegin(VertexTopologyID vertex) {
    return dlcsr.halfEdgeBegin(vertex);
  }

  EdgeHandle halfEdgeEnd(VertexTopologyID vertex) {
    return dlcsr.halfEdgeEnd(vertex);
  }

  std::uint64_t numVHosts() {
    return dlcsr.numVHosts();
  }

public:
  constexpr MirrorDistLocalCSR() noexcept = default;
  constexpr MirrorDistLocalCSR(MirrorDistLocalCSR&&) noexcept = default;
  constexpr MirrorDistLocalCSR(const MirrorDistLocalCSR&) noexcept = default;
  ~MirrorDistLocalCSR() = default;

  constexpr MirrorDistLocalCSR& operator=(const MirrorDistLocalCSR&) noexcept = default;
  constexpr MirrorDistLocalCSR& operator=(MirrorDistLocalCSR&&) noexcept = default;

  /** Official Graph APIS **/
  void deinitialize() {
    dlcsr.deinitialize();
  }

  /** size stuff **/
  std::uint64_t size() noexcept {
    return dlcsr.size() - _mirror_size;
  }
  std::uint64_t size() const noexcept {
    return dlcsr.size() - _mirror_size;
  }
  std::uint64_t sizeEdges() noexcept {
    return dlcsr.sizeEdges();
  }
  std::uint64_t sizeEdges() const noexcept {
    return dlcsr.sizeEdges();
  }
  std::uint64_t getNumEdges(VertexTopologyID vertex) {
    return dlcsr.getNumEdges(vertex);
  }
  std::uint64_t sizeMirrors() noexcept {
    return _mirror_size;
  }
  std::uint64_t sizeMirrors() const noexcept {
    return _mirror_size;
  }

  struct MirrorToMasterMap {
    MirrorToMasterMap() = default;
    MirrorToMasterMap(VertexTopologyID _mirror, VertexTopologyID _master)
        : mirror(_mirror), master(_master) {}
    VertexTopologyID mirror;
    VertexTopologyID master;
    VertexTopologyID getMirror() {
      return mirror;
    }
    VertexTopologyID getMaster() {
      return master;
    }

    bool operator==(const MirrorToMasterMap& a) noexcept {
      return a.mirror == mirror && a.master == master;
    }
  };

  /** Vertex Manipulation **/
  VertexTopologyID getTopologyID(VertexTokenID tid) {
    return dlcsr.getTopologyID(tid);
  }

  VertexTopologyID getLocalTopologyID(VertexTokenID tid) {
    return dlcsr.getLocalTopologyID(tid);
  }

  VertexTopologyID getGlobalTopologyID(VertexTokenID tid) {
    return dlcsr.getGlobalTopologyID(tid);
  }

  pando::Array<MirrorToMasterMap> getLocalMirrorToRemoteMasterOrderedTable() {
    return localMirrorToRemoteMasterOrderedTable.getLocalRef();
  }

  VertexTopologyID getTopologyIDFromIndex(std::uint64_t index) {
    return dlcsr.getTopologyIDFromIndex(index);
  }
  VertexTokenID getTokenID(VertexTopologyID tid) {
    return dlcsr.getTokenID(tid);
  }
  std::uint64_t getVertexIndex(VertexTopologyID vertex) {
    return dlcsr.getVertexIndex(vertex);
  }
  pando::Place getLocalityVertex(VertexTopologyID vertex) {
    return dlcsr.getLocalityVertex(vertex);
  }

  /** Edge Manipulation **/
  EdgeHandle mintEdgeHandle(VertexTopologyID vertex, std::uint64_t off) {
    return dlcsr.mintEdgeHandle(vertex, off);
  }
  VertexTopologyID getEdgeDst(EdgeHandle eh) {
    return dlcsr.getEdgeDst(eh);
  }

  /** Data Manipulations **/
  void setData(VertexTopologyID vertex, VertexData data) {
    dlcsr.setData(vertex, data);
  }
  pando::GlobalRef<VertexData> getData(VertexTopologyID vertex) {
    return dlcsr.getData(vertex);
  }
  void setEdgeData(EdgeHandle eh, EdgeData data) {
    dlcsr.setEdgeData(eh, data);
  }
  pando::GlobalRef<EdgeData> getEdgeData(EdgeHandle eh) {
    return dlcsr.getEdgeData(eh);
  }

  /** Ranges **/
  VertexRange vertices() {
    // This will include all mirrored vertices
    return dlcsr.vertices();
  }

  EdgeRange edges(pando::GlobalPtr<galois::Vertex> vPtr) {
    return dlcsr.edges(vPtr);
  }
  VertexDataRange vertexDataRange() noexcept {
    return dlcsr.vertexDataRange();
  }
  EdgeDataRange edgeDataRange(VertexTopologyID vertex) noexcept {
    return dlcsr.edgeDataRange(vertex);
  }

  /** Topology Modifications **/
  VertexTopologyID addVertexTopologyOnly(VertexTokenID token) {
    return dlcsr.addVertexTopologyOnly(token);
  }
  VertexTopologyID addVertex(VertexTokenID token, VertexData data) {
    return dlcsr.addVertex(token, data);
  }
  pando::Status addEdgesTopologyOnly(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts) {
    return dlcsr.addEdgesTopologyOnly(src, dsts);
  }
  pando::Status addEdges(VertexTopologyID src, pando::Vector<VertexTopologyID> dsts,
                         pando::Vector<EdgeData> data) {
    return dlcsr.addEdges(src, dsts, data);
  }
  pando::Status deleteEdges(VertexTopologyID src, pando::Vector<EdgeHandle> edges) {
    return dlcsr.deleteEdges(src, edges);
  }

  /** Gluon Graph APIS **/

  /** Size **/
  std::uint64_t getMasterSize() noexcept {
    return lift(masterRange.getLocalRef(), size);
  }
  std::uint64_t getMirrorSize() noexcept {
    return lift(mirrorRange.getLocalRef(), size);
  }

  /** Range **/
  LocalVertexRange getMasterRange() {
    return masterRange.getLocalRef();
  }
  LocalVertexRange getMirrorRange() {
    return mirrorRange.getLocalRef();
  }

  /** Host Information **/
  std::uint64_t getVirtualHostID(VertexTokenID tid) {
    return dlcsr.getVirtualHostID(tid);
  }
  std::uint64_t getPhysicalHostID(VertexTokenID tid) {
    return dlcsr.getPhysicalHostID(tid);
  }

  /**
   * @brief get vertex local dense ID
   */
  std::uint64_t getVertexLocalIndex(VertexTopologyID vertex) {
    return dlcsr.getVertexIndex(vertex);
  }

  /**
   * @brief gives the number of edges
   */

  std::uint64_t localSize(std::uint32_t host) noexcept {
    return dlcsr.localSize(host);
  }

  /**
   * @brief Sets the value of the edge provided
   */
  void setEdgeData(VertexTopologyID vertex, std::uint64_t off, EdgeData data) {
    dlcsr.setEdgeData(mintEdgeHandle(vertex, off), data);
  }

  /**
   * @brief gets the reference to the vertex provided
   */
  pando::GlobalRef<EdgeData> getEdgeData(VertexTopologyID vertex, std::uint64_t off) {
    return dlcsr.getEdgeData(mintEdgeHandle(vertex, off));
  }

  /**
   * @brief get the vertex at the end of the edge provided by vertex at the offset from the start
   */
  VertexTopologyID getEdgeDst(VertexTopologyID vertex, std::uint64_t off) {
    return dlcsr.getEdgeDst(mintEdgeHandle(vertex, off));
  }

  bool isLocal(VertexTopologyID vertex) {
    return dlcsr.isLocal(vertex);
  }

  bool isOwned(VertexTopologyID vertex) {
    return dlcsr.isOwned(vertex);
  }

  /**
   * @brief Get the local csr
   */
  pando::GlobalRef<CSR> getLocalCSR() {
    return dlcsr.getLocalCSR();
  }

  template <typename ReadVertexType, typename ReadEdgeType>
  pando::Status initializeAfterGather(
      galois::HostLocalStorage<pando::Vector<ReadVertexType>> vertexData, std::uint64_t numVertices,
      galois::HostIndexedMap<pando::Vector<pando::Vector<ReadEdgeType>>> edgeData,
      galois::HostIndexedMap<galois::HashTable<std::uint64_t, std::uint64_t>> edgeMap,
      galois::HostIndexedMap<std::uint64_t> numEdges,
      HostLocalStorage<pando::Array<std::uint64_t>> virtualToPhysical) {
    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    galois::WaitGroup wg;
    PANDO_CHECK(wg.initialize(numHosts));
    auto wgh = wg.getHandle();
    _mirror_size = 0;
    HostLocalStorage<pando::Vector<VertexTokenID>> mirrorList;
    mirrorList = this->dlcsr.getMirrorList(edgeData, virtualToPhysical);
    PANDO_CHECK(masterRange.initialize());
    PANDO_CHECK(mirrorRange.initialize());
    PANDO_CHECK(localMirrorToRemoteMasterOrderedTable.initialize());

    auto mirrorAttach = +[](galois::HostLocalStorage<pando::Vector<ReadVertexType>> vertexData,
                            HostLocalStorage<pando::Vector<VertexTokenID>> mirrorList,
                            std::uint64_t i, galois::WaitGroup::HandleType wgh) {
      pando::Vector<ReadVertexType> curVertexData = vertexData[i];
      pando::Vector<VertexTokenID> curMirrorList = mirrorList[i];
      for (uint64_t j = 0; j < lift(curMirrorList, size); j++) {
        ReadVertexType v = ReadVertexType{curMirrorList[j]};
        PANDO_CHECK(fmap(curVertexData, pushBack, v));
      }
      vertexData[i] = curVertexData;
      wgh.done();
    };
    uint64_t local_mirror_size = 0;
    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)},
                                        pando::anyPod, pando::anyCore};
      PANDO_CHECK(pando::executeOn(place, mirrorAttach, vertexData, mirrorList, i, wgh));
      local_mirror_size = lift(mirrorList[i], size);
      numVertices += local_mirror_size;
      _mirror_size += local_mirror_size;
    }
    PANDO_CHECK(wg.wait());
    wgh.add(numHosts);

    this->dlcsr.initializeAfterGather(vertexData, numVertices, edgeData, edgeMap, numEdges,
                                      virtualToPhysical);

    // Generate masterRange, mirrorRange, localMirrorToRemoteMasterOrderedTable
    auto generateMetadata = +[](MirrorDistLocalCSR<VertexType, EdgeType> mdlcsr,
                                DistLocalCSR<VertexType, EdgeType> dlcsr,
                                HostLocalStorage<pando::Vector<std::uint64_t>> mirrorList,
                                std::uint64_t i, galois::WaitGroup::HandleType wgh) {
      pando::Vector<std::uint64_t> localMirrorList = mirrorList[i];
      uint64_t mirror_size = lift(localMirrorList, size);
      CSR csrCurr = dlcsr.arrayOfCSRs[i];

      LocalVertexRange _masterRange = mdlcsr.masterRange.getLocalRef();
      _masterRange = LocalVertexRange(lift(csrCurr, vertexEdgeOffsets.begin),
                                      lift(csrCurr, size) - mirror_size);

      LocalVertexRange _mirrorRange = mdlcsr.mirrorRange.getLocalRef();
      _mirrorRange = LocalVertexRange(
          lift(csrCurr, vertexEdgeOffsets.begin) + lift(csrCurr, size) - mirror_size, mirror_size);

      pando::Array<MirrorToMasterMap> _localMirrorToRemoteMasterOrderedTable =
          mdlcsr.localMirrorToRemoteMasterOrderedTable.getLocalRef();
      fmap(_localMirrorToRemoteMasterOrderedTable, initialize, mirror_size);
      for (uint64_t j = 0; j < mirror_size; j++) {
        _localMirrorToRemoteMasterOrderedTable[j] =
            MirrorToMasterMap(fmap(dlcsr, getLocalTopologyID, localMirrorList[j]),
                              fmap(dlcsr, getGlobalTopologyID, localMirrorList[j]));
      }
      mdlcsr.masterRange.getLocalRef() = _masterRange;
      mdlcsr.mirrorRange.getLocalRef() = _mirrorRange;
      mdlcsr.localMirrorToRemoteMasterOrderedTable.getLocalRef() =
          _localMirrorToRemoteMasterOrderedTable;
      wgh.done();
    };

    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int16_t>(i)},
                                        pando::anyPod, pando::anyCore};
      PANDO_CHECK(
          pando::executeOn(place, generateMetadata, *this, this->dlcsr, mirrorList, i, wgh));
      numVertices += lift(mirrorList[i], size);
    }
    PANDO_CHECK(wg.wait());

    PANDO_CHECK_RETURN(setupCommunication());

    return pando::Status::Success;
  }

  /**
   * @brief Exchanges the mirror to master mapping from the mirror side to the maser side
   */
  pando::Status setupCommunication() {
    auto dims = pando::getPlaceDims();

    // initialize localMirrorToRemoteMasterOrderedTable
    PANDO_CHECK_RETURN(localMasterToRemoteMirrorTable.initialize());
    for (std::int16_t i = 0; i < dims.node.id; i++) {
      pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
          localMasterToRemoteMirrorMap = localMasterToRemoteMirrorTable[i];
      PANDO_CHECK_RETURN(fmap(localMasterToRemoteMirrorMap, initialize, dims.node.id));
      for (std::int16_t i = 0; i < dims.node.id; i++) {
        pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
            fmap(localMasterToRemoteMirrorMap, get, i);
        PANDO_CHECK_RETURN(fmap(mapVectorFromHost, initialize, 0));
      }
    }

    auto thisCSR = *this;
    auto state = galois::make_tpl(thisCSR, localMasterToRemoteMirrorTable);

    // push style
    // each host traverses its own localMirrorToRemoteMasterOrderedTable and send out the mapping to
    // the corresponding remote host append to the vector of vector where each vector is the mapping
    // from a specific host
    galois::doAll(
        state, localMirrorToRemoteMasterOrderedTable,
        +[](decltype(state) state,
            pando::GlobalRef<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap) {
          auto [object, localMasterToRemoteMirrorTable] = state;
          for (std::uint64_t i = 0ul; i < lift(localMirrorToRemoteMasterOrderedMap, size); i++) {
            MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, get, i);
            VertexTopologyID masterTopologyID = m.getMaster();
            VertexTokenID masterTokenID = object.getTokenID(masterTopologyID);
            std::uint64_t physicalHost = object.getPhysicalHostID(masterTokenID);

            pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
                localMasterToRemoteMirrorMap = localMasterToRemoteMirrorTable[physicalHost];
            pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
                fmap(localMasterToRemoteMirrorMap, get, pando::getCurrentPlace().node.id);

            PANDO_CHECK(fmap(mapVectorFromHost, pushBack, m));
          }
        });

    return pando::Status::Success;
  }

  /**
   * @brief For testing only
   */
  pando::GlobalRef<pando::Array<MirrorToMasterMap>> getLocalMirrorToRemoteMasterOrderedMap(
      int16_t hostId) {
    return localMirrorToRemoteMasterOrderedTable[hostId];
  }
  pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>> getLocalMasterToRemoteMirrorMap(
      uint64_t hostId) {
    return localMasterToRemoteMirrorTable[hostId];
  }

private:
  DLCSR dlcsr;
  uint64_t _mirror_size;
  galois::HostLocalStorage<LocalVertexRange> masterRange;
  galois::HostLocalStorage<LocalVertexRange> mirrorRange;
  galois::HostLocalStorage<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedTable;
  galois::HostLocalStorage<pando::Vector<pando::Vector<MirrorToMasterMap>>>
      localMasterToRemoteMirrorTable;
};

static_assert(graph_checker<MirrorDistLocalCSR<std::uint64_t, std::uint64_t>>::value);
static_assert(graph_checker<MirrorDistLocalCSR<WMDVertex, WMDEdge>>::value);

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_MIRROR_DIST_LOCAL_CSR_HPP_
