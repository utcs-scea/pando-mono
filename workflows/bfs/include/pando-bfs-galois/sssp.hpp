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
#include <pando-lib-galois/graphs/graph_traits.hpp>
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
  PerThreadVector<typename graph_traits<G>::VertexTopologyID> next;
  std::uint64_t dist;
  G graph;
};

template <typename G>
void BFSOuterLoop(BFSState<G> state,
                  pando::GlobalRef<typename graph_traits<G>::VertexTopologyID> currRef) {
  for (typename graph_traits<G>::EdgeHandle eh : state.graph.edges(currRef)) {
    countEdges.countEdge();
    typename graph_traits<G>::VertexTopologyID dst = state.graph.getEdgeDst(eh);
    uint64_t dst_data = state.graph.getData(dst);
    if (dst_data == UINT64_MAX) { // why not dst_data > state.dist?
      state.graph.setData(dst, state.dist);
      PANDO_CHECK(state.next.pushBack(dst));
    }
  }
}

template <typename G>
void BFSPerHostLoop(
    BFSState<G> state,
    pando::GlobalRef<pando::Vector<typename graph_traits<G>::VertexTopologyID>> vecRef) {
  pando::Vector<typename graph_traits<G>::VertexTopologyID> vec = vecRef;
  const auto err =
      galois::doAll(state, vec, &BFSOuterLoop<G>,
                    [](BFSState<G> state, typename graph_traits<G>::VertexTopologyID tid) {
                      return state.graph.getLocalityVertex(tid);
                    });
  PANDO_CHECK(err);
}

template <typename T>
bool IsNextIterationEmpty(galois::HostLocalStorage<pando::Vector<T>> phbfs) {
  for (pando::Vector<T> vecBFS : phbfs) {
    if (vecBFS.size() != 0)
      return false;
  }
  return true;
}

template <typename G>
pando::Status SSSP(
    G& graph, std::uint64_t src, PerThreadVector<typename graph_traits<G>::VertexTopologyID>& next,
    galois::HostLocalStorage<pando::Vector<typename graph_traits<G>::VertexTopologyID>>& phbfs) {
#ifdef DPRINTS
  std::cout << "Got into SSSP" << std::endl;
#endif

  galois::WaitGroup wg{};
  PANDO_CHECK_RETURN(wg.initialize(0));
  auto wgh = wg.getHandle();
  PANDO_CHECK_RETURN(galois::doAll(
      wgh, graph.vertexDataRange(),
      +[](pando::GlobalRef<typename graph_traits<G>::VertexData> ref) {
        ref = static_cast<std::uint64_t>(UINT64_MAX);
      }));
  PANDO_CHECK_RETURN(wg.wait());

  auto srcID = graph.getTopologyID(src);

  graph.setData(srcID, 0);

  PANDO_CHECK_RETURN(fmap(phbfs.getLocalRef(), pushBack, graph.getTopologyID(src)));

  BFSState<G> state;
  state.graph = graph;
  state.next = next;
  state.dist = 0;

  PANDO_MEM_STAT_NEW_KERNEL("BFS Start");

  while (!IsNextIterationEmpty(phbfs)) {
#ifdef DPRINTS
    std::cerr << "Iteration loop start:\t" << state.dist << std::endl;
#endif

    // Take care of last loop
    state.dist++;
    state.next.clear();

    PANDO_CHECK_RETURN(galois::doAll(wgh, state, phbfs, &BFSPerHostLoop<G>));
    PANDO_CHECK_RETURN(wg.wait());
    PANDO_MEM_STAT_NEW_KERNEL("BFS Scatter End");

    for (pando::GlobalRef<pando::Vector<typename graph_traits<G>::VertexTopologyID>> vec : phbfs) {
      liftVoid(vec, clear);
    }
    PANDO_CHECK_RETURN(state.next.hostFlattenAppend(phbfs));
    PANDO_MEM_STAT_NEW_KERNEL("BFS Reduce End");

#ifdef DPRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        phbfs, +[](pando::Vector<typename graph_traits<G>::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  wg.deinitialize();
  return pando::Status::Success;
}

template <typename G>
pando::Status MirroredSSSP(
    G& graph, std::uint64_t src, PerThreadVector<typename graph_traits<G>::VertexTopologyID>& next,
    galois::HostLocalStorage<pando::Vector<typename graph_traits<G>::VertexTopologyID>>& phbfs) {
#ifdef DPRINTS
  std::cout << "Got into SSSP" << std::endl;
#endif

  galois::WaitGroup wg{};
  PANDO_CHECK_RETURN(wg.initialize(0));
  auto wgh = wg.getHandle();
  PANDO_CHECK_RETURN(galois::doAll(
      wgh, graph.vertexDataRange(),
      +[](pando::GlobalRef<typename graph_traits<G>::VertexData> ref) {
        ref = static_cast<std::uint64_t>(UINT64_MAX);
      }));
  PANDO_CHECK_RETURN(wg.wait());

  auto srcID = graph.getTopologyID(src);

  graph.setData(srcID, 0);

  PANDO_CHECK_RETURN(fmap(phbfs.getLocalRef(), pushBack, graph.getTopologyID(src)));

  BFSState<G> state;
  state.graph = graph;
  state.next = next;
  state.dist = 0;

  PANDO_MEM_STAT_NEW_KERNEL("BFS Start");

  while (!IsNextIterationEmpty(phbfs)) {
#ifdef DPRINTS
    std::cerr << "Iteration loop start:\t" << state.dist << std::endl;
#endif

    // Take care of last loop
    state.dist++;
    state.next.clear();

    PANDO_CHECK_RETURN(galois::doAll(wgh, state, phbfs, &BFSPerHostLoop<G>));
    PANDO_CHECK_RETURN(wg.wait());
    // state.graph.sync();
    PANDO_MEM_STAT_NEW_KERNEL("BFS Scatter End");

    for (pando::GlobalRef<pando::Vector<typename graph_traits<G>::VertexTopologyID>> vec : phbfs) {
      liftVoid(vec, clear);
    }
    PANDO_CHECK_RETURN(state.next.hostFlattenAppend(phbfs));
    PANDO_MEM_STAT_NEW_KERNEL("BFS Reduce End");

#ifdef DPRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        phbfs, +[](pando::Vector<typename graph_traits<G>::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  wg.deinitialize();
  return pando::Status::Success;
}

};     // namespace galois
#endif // PANDO_BFS_GALOIS_SSSP_HPP_
