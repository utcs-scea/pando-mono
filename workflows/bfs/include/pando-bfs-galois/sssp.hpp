// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_BFS_GALOIS_SSSP_HPP_
#define PANDO_BFS_GALOIS_SSSP_HPP_

#include <pando-rt/export.h>

#include <fstream>
#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/sync/atomic.hpp>

namespace galois {

template <bool enableCount>
class CountEdges;

template <>
class CountEdges<true> {
  std::atomic<std::uint64_t> edges = 0;

public:
  void countEdge() {
    edges.fetch_add(1, std::memory_order_relaxed);
  }
  void printEdges() {
    std::cerr << "Number of Edges on host " << pando::getCurrentPlace().node.id << " is "
              << edges.load(std::memory_order_relaxed) << std::endl;
  }
  void resetCount() {
    edges = 0;
  }
};
template <>
class CountEdges<false> {
public:
  void countEdge() {}
  void printEdges() {}
  void resetCount() {}
};

#ifndef COUNTEDGE
constexpr bool COUNT_EDGE = false;
#else
constexpr bool COUNT_EDGE = true;
#endif

CountEdges<COUNT_EDGE> countEdges;

template <typename G>
struct BFSState {
  PerThreadVector<typename G::VertexTopologyID> active;
  std::uint64_t dist;
  G graph;
};

template <typename T>
bool IsactiveIterationEmpty(galois::HostLocalStorage<pando::Vector<T>> phbfs) {
  for (pando::Vector<T> vecBFS : phbfs) {
    if (vecBFS.size() != 0)
      return false;
  }
  return true;
}

template <typename G>
void BFSOuterLoop_DLCSR(BFSState<G> state, pando::GlobalRef<typename G::VertexTopologyID> currRef) {
  for (typename G::EdgeHandle eh : state.graph.edges(currRef)) {
    countEdges.countEdge();
    typename G::VertexTopologyID dst = state.graph.getEdgeDst(eh);
    uint64_t dst_data = state.graph.getData(dst);
    if (dst_data == UINT64_MAX) {
      state.graph.setData(dst, state.dist);
      PANDO_CHECK(state.active.pushBack(dst));
    }
  }
}

template <typename G>
void BFSPerHostLoop_DLCSR(BFSState<G> state,
                          pando::GlobalRef<pando::Vector<typename G::VertexTopologyID>> vecRef) {
  pando::Vector<typename G::VertexTopologyID> vec = vecRef;
  const auto err = galois::doAll(state, vec, &BFSOuterLoop_DLCSR<G>,
                                 [](BFSState<G> state, typename G::VertexTopologyID tid) {
                                   return state.graph.getLocalityVertex(tid);
                                 });
  PANDO_CHECK(err);
}

template <typename G>
pando::Status SSSP_DLCSR(
    G& graph, std::uint64_t src, PerThreadVector<typename G::VertexTopologyID>& active,
    galois::HostLocalStorage<pando::Vector<typename G::VertexTopologyID>>& phbfs) {
#ifdef DPRINTS
  std::cout << "Got into SSSP" << std::endl;
#endif

  galois::WaitGroup wg{};
  PANDO_CHECK_RETURN(wg.initialize(0));
  auto wgh = wg.getHandle();
  PANDO_CHECK_RETURN(galois::doAll(
      wgh, graph.vertexDataRange(), +[](pando::GlobalRef<typename G::VertexData> ref) {
        ref = static_cast<std::uint64_t>(UINT64_MAX);
      }));
  PANDO_CHECK_RETURN(wg.wait());

  auto srcID = graph.getTopologyID(src);

  graph.setData(srcID, 0);

  PANDO_CHECK_RETURN(fmap(phbfs.getLocalRef(), pushBack, graph.getTopologyID(src)));

  BFSState<G> state;
  state.graph = graph;
  state.active = active;
  state.dist = 0;

  PANDO_MEM_STAT_NEW_KERNEL("BFS Start");

  while (!IsactiveIterationEmpty(phbfs)) {
#ifdef DPRINTS
    std::cerr << "Iteration loop start:\t" << state.dist << std::endl;
#endif

    // Take care of last loop
    state.dist++;
    state.active.clear();

    PANDO_CHECK_RETURN(galois::doAll(wgh, state, phbfs, &BFSPerHostLoop_DLCSR<G>));
    PANDO_CHECK_RETURN(wg.wait());

    for (pando::GlobalRef<pando::Vector<typename G::VertexTopologyID>> vec : phbfs) {
      liftVoid(vec, clear);
    }
    PANDO_CHECK_RETURN(state.active.hostFlattenAppend(phbfs));

#ifdef DPRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

  PANDO_MEM_STAT_NEW_KERNEL("BFS End");

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        phbfs, +[](pando::Vector<typename G::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  wg.deinitialize();
  return pando::Status::Success;
}

void updateData(std::uint64_t val, pando::GlobalRef<std::uint64_t> ref) {
  std::uint64_t temp = pando::atomicLoad(&ref, std::memory_order_relaxed);
  do {
    if (val >= temp) {
      break;
    }
  } while (!pando::atomicCompareExchange(&ref, pando::GlobalPtr<std::uint64_t>(&temp),
                                         pando::GlobalPtr<std::uint64_t>(&val),
                                         std::memory_order_relaxed, std::memory_order_relaxed));
}

template <typename G>
void BFSOuterLoop_MDLCSR(BFSState<G> state,
                         pando::GlobalRef<typename G::VertexTopologyID> currRef) {
  typename G::VertexTopologyID temp = currRef;
  for (typename G::EdgeHandle eh : state.graph.edges(currRef)) {
    countEdges.countEdge();
    typename G::VertexTopologyID dst = state.graph.getEdgeDst(eh);
    std::uint64_t old_dst_data = state.graph.getData(dst);
    updateData(state.dist, state.graph.getData(dst));
    if (state.graph.getData(dst) != old_dst_data) {
      state.graph.setBitSet(dst);
      // PANDO_CHECK(state.active.pushBack(dst));
    }
  }
}

template <typename G>
void BFSPerHostLoop_MDLCSR(BFSState<G> state,
                           pando::GlobalRef<pando::Vector<typename G::VertexTopologyID>> vecRef) {
  pando::Vector<typename G::VertexTopologyID> vec = vecRef;
  const auto err = galois::doAll(state, vec, &BFSOuterLoop_MDLCSR<G>,
                                 [](BFSState<G> state, typename G::VertexTopologyID tid) {
                                   return state.graph.getLocalityVertex(tid);
                                 });
  PANDO_CHECK(err);
}

template <typename G>
void updateActive(BFSState<G> state) {
  galois::HostLocalStorage<pando::Array<bool>> masterBitSets = state.graph.getMasterBitSets();
  PANDO_CHECK(galois::doAll(
      state, masterBitSets,
      +[](BFSState<G> state, pando::GlobalRef<pando::Array<bool>> masterBitSet) {
        for (std::uint64_t i = 0; i < lift(masterBitSet, size); i++) {
          if (fmap(masterBitSet, operator[], i) == true) {
            typename G::LocalVertexRange localMasterRange = state.graph.getLocalMasterRange();
            typename G::VertexTopologyID masterTopologyID = *localMasterRange.begin() + i;
            PANDO_CHECK(state.active.pushBack(masterTopologyID));
          }
        }
      }));
  galois::HostLocalStorage<pando::Array<bool>> mirrorBitSets = state.graph.getMirrorBitSets();
  PANDO_CHECK(galois::doAll(
      state, mirrorBitSets,
      +[](BFSState<G> state, pando::GlobalRef<pando::Array<bool>> mirrorBitSet) {
        for (std::uint64_t i = 0; i < lift(mirrorBitSet, size); i++) {
          if (fmap(mirrorBitSet, operator[], i) == true) {
            typename G::LocalVertexRange localMirrorRange = state.graph.getLocalMirrorRange();
            typename G::VertexTopologyID mirrorTopologyID = *localMirrorRange.begin() + i;
            PANDO_CHECK(state.active.pushBack(mirrorTopologyID));
          }
        }
      }));
}

template <typename G>
pando::Status SSSP_MDLCSR(
    G& graph, std::uint64_t src, PerThreadVector<typename G::VertexTopologyID>& active,
    galois::HostLocalStorage<pando::Vector<typename G::VertexTopologyID>>& phbfs) {
#ifdef DPRINTS
  std::cout << "Got into SSSP" << std::endl;
#endif

  galois::WaitGroup wg{};
  PANDO_CHECK_RETURN(wg.initialize(0));
  auto wgh = wg.getHandle();
  PANDO_CHECK_RETURN(galois::doAll(
      wgh, graph.vertexDataRange(), +[](pando::GlobalRef<typename G::VertexData> ref) {
        ref = static_cast<std::uint64_t>(UINT64_MAX);
      }));
  PANDO_CHECK_RETURN(wg.wait());

  auto srcHost = graph.getPhysicalHostID(src);
  auto srcID = graph.getGlobalTopologyID(src);

  graph.setData(srcID, 0);

  PANDO_CHECK_RETURN(fmap(phbfs[srcHost], pushBack, srcID));

  BFSState<G> state;
  state.graph = graph;
  state.active = active;
  state.dist = 0;

  PANDO_MEM_STAT_NEW_KERNEL("BFS Start");

  while (!IsactiveIterationEmpty(phbfs)) {
#ifdef DPRINTS
    std::cerr << "Iteration loop start:\t" << state.dist << std::endl;
#endif
    /*
        for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
          std::cout << "Host " << nodeId << ":" << std::endl;
          std::cout << "masters" << std::endl;
          pando::GlobalRef<typename G::LocalVertexRange> masterRange =
       state.graph.getMasterRange(nodeId); for (typename G::VertexTopologyID masterTopologyID =
       *lift(masterRange, begin); masterTopologyID < *lift(masterRange, end); masterTopologyID++) {
            typename G::VertexTokenID masterTokenID = graph.getTokenID(masterTopologyID);
            typename G::VertexData masterData = graph.getData(masterTopologyID);
            std::cout << "token ID = " << masterTokenID << ", data = " << masterData << std::endl;
          }
        }
    */
    // Take care of last loop
    state.dist++;
    state.active.clear();

    PANDO_CHECK_RETURN(galois::doAll(wgh, state, phbfs, &BFSPerHostLoop_MDLCSR<G>));
    PANDO_CHECK_RETURN(wg.wait());

    state.graph.sync(updateData);
    updateActive(state);
    state.graph.resetBitSets();

    for (pando::GlobalRef<pando::Vector<typename G::VertexTopologyID>> vec : phbfs) {
      liftVoid(vec, clear);
    }
    PANDO_CHECK_RETURN(state.active.hostFlattenAppend(phbfs));

#ifdef DPRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

  PANDO_MEM_STAT_NEW_KERNEL("BFS End");

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        phbfs, +[](pando::Vector<typename G::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  wg.deinitialize();
  return pando::Status::Success;
}

};     // namespace galois
#endif // PANDO_BFS_GALOIS_SSSP_HPP_
