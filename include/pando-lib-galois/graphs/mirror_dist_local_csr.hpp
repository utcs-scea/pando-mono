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

  /** Official Graph APIs **/
  void deinitialize() {
    dlcsr.deinitialize();
  }

  /** size stuff **/
  std::uint64_t size() noexcept {
    return _master_size;
  }
  std::uint64_t size() const noexcept {
    return _master_size;
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

  /** Vertex Manipulation **/
  VertexTopologyID getTopologyID(VertexTokenID tid) {
    return dlcsr.getTopologyID(tid);
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

    setBitSet(vertex);
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

  /** Gluon Graph Structs **/
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

  /** Gluon Graph APIs **/

  /**
   * @brief returns size of all mirror vertices in the distributed graph
   */
  std::uint64_t sizeMirrors() noexcept {
    return _mirror_size;
  }
  /**
   * @brief returns size of all mirror vertices in the distributed graph
   */
  std::uint64_t sizeMirrors() const noexcept {
    return _mirror_size;
  }
  /**
   * @brief returns size of master vertices in the local graph
   */
  std::uint64_t getMasterSize() noexcept {
    return lift(masterRange.getLocalRef(), size);
  }
  /**
   * @brief returns size of master vertices in the local graph
   */
  std::uint64_t getMasterSize() const noexcept {
    return lift(masterRange.getLocalRef(), size);
  }
  /**
   * @brief returns size of mirror vertices in the local graph
   */
  std::uint64_t getMirrorSize() noexcept {
    return lift(mirrorRange.getLocalRef(), size);
  }
  /**
   * @brief returns size of mirror vertices in the local graph
   */
  std::uint64_t getMirrorSize() const noexcept {
    return lift(mirrorRange.getLocalRef(), size);
  }

  /**
   * @brief returns local topology ID of a mirror vertex
   */
  Pair<VertexTopologyID, bool> getLocalTopologyID(VertexTokenID tid) {
    return dlcsr.getLocalTopologyID(tid);
  }
  /**
   * @brief returns global (remote) topology ID of the master vertex which a mirror vertex
   * corresponds to
   */
  VertexTopologyID getGlobalTopologyID(VertexTokenID tid) {
    return dlcsr.getGlobalTopologyID(tid);
  }
  /**
   * @brief returns local topology ID of a master vertex from the index
   */
  VertexTopologyID getMasterTopologyIDFromIndex(std::uint64_t index) {
    LocalVertexRange localMasterRange = getLocalMasterRange();
    if (index < localMasterRange.size()) {
      VertexTopologyID masterTopologyID = *localMasterRange.begin() + index;
      return masterTopologyID;
    } else {
      PANDO_ABORT("INDEX FOR MASTER OUT OF RANGE");
    }
  }
  /**
   * @brief returns local topology ID of a mirror vertex from the index
   */
  VertexTopologyID getMirrorTopologyIDFromIndex(std::uint64_t index) {
    LocalVertexRange localMirrorRange = getLocalMirrorRange();
    if (index < localMirrorRange.size()) {
      VertexTopologyID mirrorTopologyID = *localMirrorRange.begin() + index;
      return mirrorTopologyID;
    } else {
      PANDO_ABORT("INDEX FOR MASTER OUT OF RANGE");
    }
  }
  /**
   * @brief get the index of a vertex within a specific vertex range
   */
  std::uint64_t getIndex(VertexTopologyID vertex, LocalVertexRange vertexList) {
    if (*vertexList.begin() <= vertex && *vertexList.end() > vertex) {
      return static_cast<uint64_t>(vertex - *vertexList.begin());
    } else {
      PANDO_ABORT("ILLEGAL SUBTRACTION OF POINTERS");
    }
  }

  /**
   * @brief sets the data of a vertex but does not update the bit set
   */
  void setDataOnly(VertexTopologyID vertex, VertexData data) {
    dlcsr.setData(vertex, data);
  }

  /**
   * @brief returns master range of the local graph
   */
  LocalVertexRange getLocalMasterRange() {
    return masterRange.getLocalRef();
  }
  /**
   * @brief returns mirror range of the local graph
   */
  LocalVertexRange getLocalMirrorRange() {
    return mirrorRange.getLocalRef();
  }

  /**
   * @brief returns if a vertex is local or not
   */
  bool isLocal(VertexTopologyID vertex) {
    return dlcsr.isLocal(vertex);
  }
  /**
   * @brief returns if a vertex is owned or not
   */
  bool isOwned(VertexTopologyID vertex) {
    return dlcsr.isOwned(vertex);
  }
  /**
   * @brief returns if a vertex is a local master or not
   */
  bool isMaster(VertexTopologyID vertex) {
    if (isLocal(vertex)) {
      auto it = getLocalMasterRange();
      if (*it.begin() <= vertex && vertex < *it.end()) {
        return true;
      } else {
        return false;
      }
    } else {
      PANDO_ABORT("INPUT NEEDS TO BE LOCAL VERTEX TOPOLOGY ID");
    }
  }
  /**
   * @brief returns if a vertex is a local mirror or not
   */
  bool isMirror(VertexTopologyID vertex) {
    if (isLocal(vertex)) {
      auto it = getLocalMirrorRange();
      if (*it.begin() <= vertex && vertex < *it.end()) {
        return true;
      } else {
        return false;
      }
    } else {
      PANDO_ABORT("INPUT NEEDS TO BE LOCAL VERTEX TOPOLOGY ID");
    }
  }

  /**
   * @brief returns the master bit sets of all hosts
   */
  galois::HostLocalStorage<pando::Array<bool>> getMasterBitSets() {
    return masterBitSets;
  }
  /**
   * @brief returns the mirror bit sets of all hosts
   */
  galois::HostLocalStorage<pando::Array<bool>> getMirrorBitSets() {
    return mirrorBitSets;
  }
  /**
   * @brief returns the master bit set of the local graph
   */
  pando::GlobalRef<pando::Array<bool>> getLocalMasterBitSet() {
    return masterBitSets.getLocalRef();
  }
  /**
   * @brief returns the master bit set of the local graph
   */
  pando::GlobalRef<pando::Array<bool>> getLocalMirrorBitSet() {
    return mirrorBitSets.getLocalRef();
  }
  /**
   * @brief reset the master bit sets of all hosts
   */
  void resetMasterBitSets() {
    galois::doAll(
        masterBitSets, +[](pando::GlobalRef<pando::Array<bool>> masterBitSet) {
          fmapVoid(masterBitSet, fill, false);
        });
  }
  /**
   * @brief reset the mirror bit sets of all hosts
   */
  void resetMirrorBitSets() {
    galois::doAll(
        mirrorBitSets, +[](pando::GlobalRef<pando::Array<bool>> mirrorBitSet) {
          fmapVoid(mirrorBitSet, fill, false);
        });
  }
  /**
   * @brief reset bit sets of all hosts
   */
  void resetBitSets() {
    resetMasterBitSets();
    resetMirrorBitSets();
  }
  /**
   * @brief reset the master bit set of the local graph
   */
  void resetLocalMasterBitSet() {
    fmap(getLocalMasterBitSet(), fill, false);
  }
  /**
   * @brief reset the mirror bit set of the local graph
   */
  void resetLocalMirrorBitSet() {
    fmap(getLocalMirrorBitSet(), fill, false);
  }
  /**
   * @brief set the bit set of a vertex
   */
  void setBitSet(VertexTopologyID vertex) {
    if (isLocal(vertex)) { // vertex is local
      // set bit set according to mirror or master
      if (isMirror(vertex)) {
        std::uint64_t index = getIndex(vertex, getLocalMirrorRange());
        pando::GlobalRef<pando::Array<bool>> mirrorBitSet = mirrorBitSets.getLocalRef();
        fmap(mirrorBitSet, operator[], index) = true;
      } else if (isMaster(vertex)) {
        std::uint64_t index = getIndex(vertex, getLocalMasterRange());
        pando::GlobalRef<pando::Array<bool>> masterBitSet = masterBitSets.getLocalRef();
        fmap(masterBitSet, operator[], index) = true;
      }
    } else { // vertex is remote
      VertexTokenID tokenId = getTokenID(vertex);
      std::uint64_t physicalHost = getPhysicalHostID(tokenId);
      pando::GlobalRef<pando::Array<bool>> masterBitSet = masterBitSets[physicalHost];
      std::uint64_t index = getIndex(vertex, getMasterRange(physicalHost));
      fmap(masterBitSet, operator[], index) = true;
    }
  }
  /**
   * @brief returns the dirty master topology IDs corresponding to the master bit set of the local
   * graph
   */
  pando::Vector<VertexTopologyID> getLocalDirtyMasters() {
    pando::Vector<VertexTopologyID> dirtyMasterTopology;
    pando::GlobalRef<pando::Array<bool>> masterBitSet = masterBitSets.getLocalRef();
    for (std::uint64_t i = 0ul; i < lift(masterBitSet, size); i++) {
      if (fmap(masterBitSet, operator[], i) == true) {
        VertexTopologyID masterTopologyID = getMasterTopologyIDFromIndex(i);
        dirtyMasterTopology.pushBack(masterTopologyID);
      }
    }

    return dirtyMasterTopology;
  }
  /**
   * @brief returns the dirty mirror topology IDs corresponding to the mirror bit set of the local
   * graph
   */
  pando::Vector<VertexTopologyID> getLocalDirtyMirrors() {
    pando::Vector<VertexTopologyID> dirtyMirrorTopology;
    pando::GlobalRef<pando::Array<bool>> mirrorBitSet = mirrorBitSets.getLocalRef();
    for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
      if (fmap(mirrorBitSet, operator[], i) == true) {
        VertexTopologyID mirrorTopologyID = getMirrorTopologyIDFromIndex(i);
        dirtyMirrorTopology.pushBack(mirrorTopologyID);
      }
    }

    return dirtyMirrorTopology;
  }
  /**
   * @brief returns the dirty vertex topology IDs corresponding to the bit sets of the local graph
   *        order is master then mirror
   */
  pando::Vector<VertexTopologyID> getLocalDirtyVertices() {
    pando::Vector<VertexTopologyID> dirtyVertexTopology;
    pando::GlobalRef<pando::Array<bool>> masterBitSet = masterBitSets.getLocalRef();
    for (std::uint64_t i = 0ul; i < lift(masterBitSet, size); i++) {
      if (fmap(masterBitSet, operator[], i) == true) {
        VertexTopologyID masterTopologyID = getMasterTopologyIDFromIndex(i);
        dirtyVertexTopology.pushBack(masterTopologyID);
      }
    }
    pando::GlobalRef<pando::Array<bool>> mirrorBitSet = mirrorBitSets.getLocalRef();
    for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
      if (fmap(mirrorBitSet, operator[], i) == true) {
        VertexTopologyID mirrorTopologyID = getMirrorTopologyIDFromIndex(i);
        dirtyVertexTopology.pushBack(mirrorTopologyID);
      }
    }

    return dirtyVertexTopology;
  }

  /**
   * @brief gives the number of edges
   */
  std::uint64_t localSize(std::uint32_t host) noexcept {
    return dlcsr.localSize(host);
  }
  /**
   * @brief get vertex local dense ID
   */
  std::uint64_t getVertexLocalIndex(VertexTopologyID vertex) {
    return dlcsr.getVertexIndex(vertex);
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
  /**
   * @brief get the local csr
   */
  pando::GlobalRef<CSR> getLocalCSR() {
    return dlcsr.getLocalCSR();
  }

  /**
   * @brief get the local mirror to master map
   */
  pando::Array<MirrorToMasterMap> getLocalMirrorToMasterMap() {
    return localMirrorToRemoteMasterOrderedTable.getLocalRef();
  }
  /**
   * @brief get the local master to mirror map
   */
  pando::Vector<pando::Vector<MirrorToMasterMap>> getLocalMasterToMirrorMap() {
    return localMasterToRemoteMirrorTable.getLocalRef();
  }

  /**
   * @brief get the virtual host ID
   */
  std::uint64_t getVirtualHostID(VertexTokenID tid) {
    return dlcsr.getVirtualHostID(tid);
  }
  /**
   * @brief get the physical host ID
   */
  std::uint64_t getPhysicalHostID(VertexTokenID tid) {
    return dlcsr.getPhysicalHostID(tid);
  }

  /**
   * @brief Reduces the updated mirror values to the corresponding master values
   */
  template <typename Func>
  void reduce(Func func) {
    WaitGroup wg;
    PANDO_CHECK(wg.initialize(0));
    WaitGroup::HandleType wgh = wg.getHandle();
    auto thisMDLCSR = *this;
    auto state = galois::make_tpl(thisMDLCSR, func, wgh);

    PANDO_CHECK(galois::doAll(
        wgh, state, localMirrorToRemoteMasterOrderedTable,
        +[](decltype(state) state,
            pando::GlobalRef<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap) {
          auto [thisMDLCSR, func, wgh] = state;

          pando::GlobalRef<pando::Array<bool>> mirrorBitSet = thisMDLCSR.getLocalMirrorBitSet();

          for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
            bool dirty = fmap(mirrorBitSet, operator[], i);
            if (dirty) {
              // obtain the local mirror vertex data
              VertexTopologyID mirrorTopologyID = thisMDLCSR.getMirrorTopologyIDFromIndex(i);
              // a copy
              VertexData mirrorData = thisMDLCSR.getData(mirrorTopologyID);

              // obtain the corresponding remote master information
              MirrorToMasterMap map = fmap(localMirrorToRemoteMasterOrderedMap, operator[], i);
              VertexTopologyID masterTopologyID = map.getMaster();
              // atomic function signature: func(VertexData mirror, pando::GlobalRef<VertexData>
              // master) apply the function
              wgh.addOne();
              PANDO_CHECK(executeOn(
                  localityOf(masterTopologyID),
                  +[](Func func, decltype(thisMDLCSR) thisMDLCSR, VertexTopologyID masterTopologyID,
                      VertexData mirrorData, WaitGroup::HandleType wgh) {
                    pando::GlobalRef<VertexData> masterData = thisMDLCSR.getData(masterTopologyID);
                    VertexData oldMasterData = masterData;
                    func(mirrorData, masterData);
                    if (masterData != oldMasterData) {
                      // set the remote master bit set
                      pando::GlobalRef<pando::Array<bool>> masterBitSet =
                          thisMDLCSR.getLocalMasterBitSet();
                      std::uint64_t index =
                          thisMDLCSR.getIndex(masterTopologyID, thisMDLCSR.getLocalMasterRange());
                      fmap(masterBitSet, operator[], index) = true;
                    }
                    wgh.done();
                  },
                  func, thisMDLCSR, masterTopologyID, mirrorData, wgh));
              //  master data updated
            }
          }
        }));
    PANDO_CHECK(wg.wait());
    wg.deinitialize();
  }
  /**
   * @brief Broadcast the updated master values to the corresponding mirror values
   */
  void broadcast() {
    WaitGroup wg;
    PANDO_CHECK(wg.initialize(0));
    WaitGroup::HandleType wgh = wg.getHandle();

    auto thisMDLCSR = *this;
    auto state = galois::make_tpl(thisMDLCSR, wgh);

    PANDO_CHECK(galois::doAll(
        wgh, state, localMasterToRemoteMirrorTable,
        +[](decltype(state) state, pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
                                       localMasterToRemoteMirrorMap) {
          auto [thisMDLCSR, wgh] = state;

          pando::GlobalRef<pando::Array<bool>> masterBitSet = thisMDLCSR.getLocalMasterBitSet();

          std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);

          for (std::uint64_t nodeId = 0ul; nodeId < numHosts; nodeId++) {
            pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
                fmap(localMasterToRemoteMirrorMap, operator[], nodeId);
            for (std::uint64_t i = 0ul; i < lift(mapVectorFromHost, size); i++) {
              MirrorToMasterMap map = fmap(mapVectorFromHost, operator[], i);
              VertexTopologyID masterTopologyID = map.getMaster();
              std::uint64_t index =
                  thisMDLCSR.getIndex(masterTopologyID, thisMDLCSR.getLocalMasterRange());
              bool dirty = fmap(masterBitSet, operator[], index);
              if (dirty) {
                // obtain the local master vertex data
                VertexData masterData = thisMDLCSR.getData(masterTopologyID);

                // obtain the corresponding remote mirror information
                VertexTopologyID mirrorTopologyID = map.getMirror();
                wgh.addOne();
                PANDO_CHECK(executeOn(
                    localityOf(mirrorTopologyID),
                    +[](decltype(thisMDLCSR) thisMDLCSR, VertexTopologyID mirrorTopologyID,
                        VertexData masterData, WaitGroup::HandleType wgh) {
                      pando::GlobalRef<VertexData> mirrorData =
                          thisMDLCSR.getData(mirrorTopologyID);
                      VertexData oldMirrorData = mirrorData;
                      mirrorData = masterData;
                      if (mirrorData != oldMirrorData) {
                        // set the remote mirror bit set
                        pando::GlobalRef<pando::Array<bool>> mirrorBitSet =
                            thisMDLCSR.getLocalMirrorBitSet();
                        std::uint64_t index =
                            thisMDLCSR.getIndex(mirrorTopologyID, thisMDLCSR.getLocalMirrorRange());
                        fmap(mirrorBitSet, operator[], index) = true;
                      }
                      wgh.done();
                    },
                    thisMDLCSR, mirrorTopologyID, masterData, wgh));
              }
            }
          }
        }));
    PANDO_CHECK(wg.wait());
    wg.deinitialize();
  }
  /**
   * @brief Synchronize master and mirror values among hosts
   */
  template <typename Func, bool REDUCE = true, bool BROADCAST = true>
  void sync(Func func) {
    if (REDUCE) {
      reduce(func);
    }
    if (BROADCAST) {
      broadcast();
    }
  }

  template <typename ReadVertexType, typename ReadEdgeType>
  pando::Status initializeAfterGather(
      galois::HostLocalStorage<pando::Vector<ReadVertexType>> vertexData, std::uint64_t numVertices,
      galois::HostLocalStorage<pando::Vector<pando::Vector<ReadEdgeType>>> edgeData,
      galois::HostLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> edgeMap,
      galois::HostIndexedMap<std::uint64_t> numEdges,
      HostLocalStorage<pando::Array<std::uint64_t>> virtualToPhysical) {
    std::uint64_t numHosts = static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
    galois::WaitGroup wg;
    PANDO_CHECK(wg.initialize(numHosts));
    auto wgh = wg.getHandle();
    _master_size = numVertices;
    _mirror_size = 0;
    HostLocalStorage<pando::Vector<VertexTokenID>> mirrorList;
    mirrorList = this->dlcsr.getMirrorList(edgeData, virtualToPhysical);
    PANDO_CHECK(masterRange.initialize());
    PANDO_CHECK(mirrorRange.initialize());
    PANDO_CHECK(localMirrorToRemoteMasterOrderedTable.initialize());

    auto mirrorAttach = +[](HostLocalStorage<pando::Vector<ReadVertexType>> vertexData,
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
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int64_t>(i)},
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
      CSR csrCurr = dlcsr.getCSR(i);

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
            MirrorToMasterMap(std::get<0>(fmap(dlcsr, getLocalTopologyID, localMirrorList[j])),
                              fmap(dlcsr, getGlobalTopologyID, localMirrorList[j]));
      }
      mdlcsr.masterRange.getLocalRef() = _masterRange;
      mdlcsr.mirrorRange.getLocalRef() = _mirrorRange;
      mdlcsr.localMirrorToRemoteMasterOrderedTable.getLocalRef() =
          _localMirrorToRemoteMasterOrderedTable;
      wgh.done();
    };

    for (std::uint64_t i = 0; i < numHosts; i++) {
      pando::Place place = pando::Place{pando::NodeIndex{static_cast<std::int64_t>(i)},
                                        pando::anyPod, pando::anyCore};
      PANDO_CHECK(
          pando::executeOn(place, generateMetadata, *this, this->dlcsr, mirrorList, i, wgh));
      numVertices += lift(mirrorList[i], size);
    }
    PANDO_CHECK(wg.wait());

    // exchange mapping to set up communication
    PANDO_CHECK_RETURN(setupCommunication());

    // initialize bit sets for mirror and master
    PANDO_CHECK_RETURN(mirrorBitSets.initialize());
    PANDO_CHECK_RETURN(masterBitSets.initialize());
    auto state = galois::make_tpl(masterRange, mirrorRange, mirrorBitSets);
    PANDO_CHECK(galois::doAll(
        wgh, state, masterBitSets,
        +[](decltype(state) state, pando::GlobalRef<pando::Array<bool>> masterBitSet) {
          auto [masterRange, mirrorRange, mirrorBitSets] = state;
          pando::GlobalRef<pando::Array<bool>> mirrorBitSet = mirrorBitSets.getLocalRef();
          PANDO_CHECK(fmap(mirrorBitSet, initialize, lift(mirrorRange.getLocalRef(), size)));
          PANDO_CHECK(fmap(masterBitSet, initialize, lift(masterRange.getLocalRef(), size)));
          fmapVoid(mirrorBitSet, fill, false);
          fmapVoid(masterBitSet, fill, false);
        }));
    PANDO_CHECK(wg.wait());
    // wg.deinitialize();
    return pando::Status::Success;
  }

  /**
   * @brief Exchanges the mirror to master mapping from the mirror side to the maser side
   */
  pando::Status setupCommunication() {
    auto dims = pando::getPlaceDims();

    // initialize localMirrorToRemoteMasterOrderedTable
    PANDO_CHECK_RETURN(localMasterToRemoteMirrorTable.initialize());
    for (std::int64_t i = 0; i < dims.node.id; i++) {
      pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
          localMasterToRemoteMirrorMap = localMasterToRemoteMirrorTable[i];
      PANDO_CHECK_RETURN(fmap(localMasterToRemoteMirrorMap, initialize, dims.node.id));
      for (std::int64_t i = 0; i < dims.node.id; i++) {
        pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
            fmap(localMasterToRemoteMirrorMap, operator[], i);
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
            MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, operator[], i);
            VertexTopologyID masterTopologyID = m.getMaster();
            VertexTokenID masterTokenID = object.getTokenID(masterTopologyID);
            std::uint64_t physicalHost = object.getPhysicalHostID(masterTokenID);

            pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
                localMasterToRemoteMirrorMap = localMasterToRemoteMirrorTable[physicalHost];
            pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
                fmap(localMasterToRemoteMirrorMap, operator[], pando::getCurrentPlace().node.id);

            PANDO_CHECK(fmap(mapVectorFromHost, pushBack, m));
          }
        });

    return pando::Status::Success;
  }

  /** Testing Purpose **/
  pando::GlobalRef<pando::Array<MirrorToMasterMap>> getLocalMirrorToRemoteMasterOrderedMap(
      int64_t hostId) {
    return localMirrorToRemoteMasterOrderedTable[hostId];
  }
  pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>> getLocalMasterToRemoteMirrorMap(
      uint64_t hostId) {
    return localMasterToRemoteMirrorTable[hostId];
  }
  pando::GlobalRef<pando::Array<bool>> getMasterBitSet(int64_t hostId) {
    return masterBitSets[hostId];
  }
  pando::GlobalRef<pando::Array<bool>> getMirrorBitSet(int64_t hostId) {
    return mirrorBitSets[hostId];
  }
  pando::GlobalRef<LocalVertexRange> getMasterRange(int64_t hostId) {
    return masterRange[hostId];
  }
  pando::GlobalRef<LocalVertexRange> getMirrorRange(int64_t hostId) {
    return mirrorRange[hostId];
  }

private:
  DLCSR dlcsr;
  uint64_t _master_size;
  uint64_t _mirror_size;
  galois::HostLocalStorage<LocalVertexRange> masterRange;
  galois::HostLocalStorage<LocalVertexRange> mirrorRange;
  galois::HostLocalStorage<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedTable;
  galois::HostLocalStorage<pando::Vector<pando::Vector<MirrorToMasterMap>>>
      localMasterToRemoteMirrorTable;

  galois::HostLocalStorage<pando::Array<bool>> mirrorBitSets;
  galois::HostLocalStorage<pando::Array<bool>> masterBitSets;
};

static_assert(graph_checker<MirrorDistLocalCSR<std::uint64_t, std::uint64_t>>::value);
static_assert(graph_checker<MirrorDistLocalCSR<WMDVertex, WMDEdge>>::value);

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_MIRROR_DIST_LOCAL_CSR_HPP_
