// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_BFS_GALOIS_SSSP_HPP_
#define PANDO_BFS_GALOIS_SSSP_HPP_

#include <pando-rt/export.h>

#include <cassert>
#include <fstream>
#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/thread_local_vector.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/drv_info.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/sync/atomic.hpp>

namespace bfs {

using galois::HostLocalStorage;
using galois::ThreadLocalVector;
using galois::WaitGroup;

template <bool enableCount>
class CountEdges;

template <>
class CountEdges<true> {
  std::atomic<std::uint64_t> edges = 0;

public:
  inline void countEdge() {
    edges.fetch_add(1, std::memory_order_relaxed);
  }

  inline void printEdges() {
    std::cerr << "Number of Edges on host " << pando::getCurrentPlace().node.id << " is "
              << edges.load(std::memory_order_relaxed) << std::endl;
  }

  inline void resetCount() {
    edges = 0;
  }
};
template <>
class CountEdges<false> {
public:
  inline void countEdge() {}
  inline void printEdges() {}
  inline void resetCount() {}
};

#ifndef COUNTEDGE
constexpr bool COUNT_EDGE = false;
#else
constexpr bool COUNT_EDGE = true;
#endif

extern CountEdges<COUNT_EDGE> countEdges;

template <typename G>
struct BFSState {
  ThreadLocalVector<typename G::VertexTopologyID> active;
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

using DLCSR = galois::DistLocalCSR<std::uint64_t, std::uint64_t>;
template <>
void BFSPerHostLoop_DLCSR<DLCSR>(
    BFSState<DLCSR> state,
    pando::GlobalRef<pando::Vector<typename DLCSR::VertexTopologyID>> vecRef);
template <typename G>
pando::Status SSSP_DLCSR(
    G& graph, std::uint64_t src, ThreadLocalVector<typename G::VertexTopologyID>& active,
    galois::HostLocalStorage<pando::Vector<typename G::VertexTopologyID>>& phbfs) {
#ifdef DEBUG_PRINTS
  std::cerr << "Got into SSSP" << std::endl;
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

#ifdef PANDO_STAT_TRACE_ENABLE
  PANDO_CHECK(galois::doAllExplicitPolicy<SchedulerPolicy::INFER_RANDOM_CORE>(
      wgh, phbfs, +[](pando::Vector<typename G::VertexTopologyID>) {
        PANDO_MEM_STAT_NEW_KERNEL("BFS Start");
      }));
  PANDO_CHECK(wg.wait());
#endif

  while (!IsactiveIterationEmpty(phbfs)) {
#ifdef DEBUG_PRINTS
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

    PANDO_DRV_INCREMENT_PHASE();

#ifdef DEBUG_PRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

#ifdef PANDO_STAT_TRACE_ENABLE
  PANDO_CHECK(galois::doAllExplicitPolicy<SchedulerPolicy::INFER_RANDOM_CORE>(
      wgh, phbfs, +[](pando::Vector<typename G::VertexTopologyID>) {
        PANDO_MEM_STAT_NEW_KERNEL("BFS END");
      }));
  PANDO_CHECK(wg.wait());
#endif
  PANDO_DRV_SET_STAGE_OTHER();

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        phbfs, +[](pando::Vector<typename G::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  active = state.active;
  wg.deinitialize();
  return pando::Status::Success;
}

void updateData(std::uint64_t val, pando::GlobalRef<std::uint64_t> ref);

template <typename T>
using R = pando::GlobalRef<T>;
template <typename T>
using P = pando::GlobalPtr<T>;
template <typename G>
using VTopID = typename G::VertexTopologyID;
template <typename G>
using MDInnerWorkList = pando::Vector<VTopID<G>>;
template <typename G>
using MDWorkList = pando::Array<MDInnerWorkList<G>>;

template <typename G>
bool isWorkListEmpty(MDWorkList<G> worklist) {
  for (MDInnerWorkList<G> vec : worklist) {
    if (vec.size())
      return false;
  }
  return true;
}

template <typename G>
bool SSSPFunctor(G& graph, R<MDInnerWorkList<G>> toWrite, VTopID<G> vertex) {
#ifndef NDEBUG
  assert(localityOf(vertex).node.id == pando::getCurrentPlace().node.id);
#endif
  bool ready = false;
  auto currDist = graph.getData(vertex) + 1;
  for (typename G::EdgeHandle eh : graph.edges(vertex)) {
    countEdges.countEdge();
    typename G::VertexTopologyID dst = graph.getEdgeDst(eh);
    R<std::uint64_t> currData = graph.getData(dst);
    std::uint64_t oldData = currData;
    if (oldData > currDist) {
      updateData(currDist, currData);
      PANDO_CHECK(fmap(toWrite, pushBack, dst));
      graph.setBitSet(dst);
      ready = true;
    }
  }
  return ready;
}

template <typename G>
pando::Status MDLCSRLocal(G& graph, MDWorkList<G> toRead, MDWorkList<G> toWrite) {
  P<bool> ready;
  pando::LocalStorageGuard<bool> guardReady(ready, 1);
  *ready = false;
  WaitGroup wg;
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();
  while (!*ready) {
    *ready = true;
    for (R<MDInnerWorkList<G>> toRun : toRead) {
      MDInnerWorkList<G> vec = toRun;
      liftVoid(toRun, clear);
      auto innerState = galois::make_tpl(graph, toWrite, ready);
      PANDO_CHECK(doAll(
          wgh, innerState, vec, +[](decltype(innerState) innerState, VTopID<G> vertex) {
            auto [graph, toWrite, ready] = innerState;
            const std::uint64_t threadPerHostIdx =
                galois::getCurrentThreadIdx() % galois::getThreadsPerHost();
            if (!SSSPFunctor(graph, toWrite[threadPerHostIdx], vertex))
              *ready = false;
          }));
    }
    PANDO_CHECK(wg.wait());
    std::swap(toRead, toWrite);
  }
  wg.deinitialize();
  return pando::Status::Success;
}

template <typename G>
bool updateActive(G& graph, MDWorkList<G> toRead, const pando::Array<bool>& masterBitSet) {
  bool active = false;
  for (std::uint64_t i = 0; i < masterBitSet.size(); i++) {
    if (masterBitSet[i]) {
      active = true;
      PANDO_CHECK(fmap(toRead[0], pushBack, graph.getMasterTopologyIDFromIndex(i)));
    }
  }
  return active;
}

template <typename G>
pando::Status SSSPMDLCSR(G& graph, std::uint64_t src, HostLocalStorage<MDWorkList<G>>& toRead,
                         HostLocalStorage<MDWorkList<G>>& toWrite, P<bool> active) {
#ifdef DEBUG_PRINTS
  std::cerr << "Got into SSSP" << std::endl;
#endif
  galois::WaitGroup wg{};
  PANDO_CHECK_RETURN(wg.initialize(0));
  auto wgh = wg.getHandle();
  PANDO_CHECK_RETURN(galois::doAll(
      wgh, graph.vertexDataRange(), +[](pando::GlobalRef<typename G::VertexData> ref) {
        ref = static_cast<std::uint64_t>(UINT64_MAX);
      }));
  PANDO_CHECK_RETURN(wg.wait());

  auto initialState = galois::make_tpl(graph, src);
  PANDO_CHECK(galois::doAll(
      wgh, initialState, toRead, +[](decltype(initialState) state, MDWorkList<G> toRead) {
        auto [graph, src] = state;
        auto [srcID, found] = graph.getLocalTopologyID(src);
        if (found) {
          graph.setDataOnly(srcID, 0);

          std::int64_t srcHost = graph.getPhysicalHostID(src);
          if (srcHost == pando::getCurrentPlace().node.id) {
            PANDO_CHECK(fmap(toRead[0], pushBack, srcID));
          }
        }
      }));
  PANDO_CHECK_RETURN(wg.wait());

#ifdef DEBUG_PRINTS
  std::uint64_t srcHost = graph.getPhysicalHostID(src);
  std::cerr << "Source is on host " << srcHost << std::endl;
#endif

#ifdef PANDO_STAT_TRACE_ENABLE
  PANDO_CHECK(galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
      wgh, toRead, +[](MDWorkList<G>) {
        PANDO_MEM_STAT_NEW_KERNEL("BFS Start");
      }));
  PANDO_CHECK(wg.wait());
#endif

  *active = true;
  while (*active) {
#ifdef DEBUG_PRINTS
    std::cerr << "Iteration loop start:\t" << state.dist << std::endl;
#endif
    *active = false;

    auto state = galois::make_tpl(graph, toWrite);
    PANDO_CHECK_RETURN(galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
        wgh, state, toRead, +[](decltype(state) state, MDWorkList<G> toRead) {
          auto [graph, toWrite] = state;
          MDLCSRLocal<G>(graph, toRead, toWrite.getLocalRef());
        }));
    PANDO_CHECK_RETURN(wg.wait());

    PANDO_DRV_SET_STAGE_EXEC_COMM();
    graph.template sync<decltype(updateData), true, false>(updateData);

    galois::HostLocalStorage<pando::Array<bool>> masterBitSets = graph.getMasterBitSets();
    auto activeState = galois::make_tpl(graph, toRead, active);
    PANDO_CHECK_RETURN(galois::doAll(
        wgh, activeState, masterBitSets,
        +[](decltype(activeState) activeState, pando::Array<bool> masterBitSet) {
          auto [graph, toRead, active] = activeState;
          if (updateActive(graph, toRead.getLocalRef(), masterBitSet)) {
            *active = true;
          }
        }));

    PANDO_CHECK_RETURN(wg.wait());
    graph.resetBitSets();
    PANDO_DRV_SET_STAGE_EXEC_COMP();
    PANDO_DRV_INCREMENT_PHASE();

#ifdef DEBUG_PRINTS
    std::cerr << "Iteration loop end:\t" << state.dist - 1 << std::endl;
#endif
  }

#ifdef PANDO_STAT_TRACE_ENABLE
  PANDO_CHECK(galois::doAllExplicitPolicy<galois::SchedulerPolicy::INFER_RANDOM_CORE>(
      wgh, toRead, +[](MDWorkList<G>) {
        PANDO_MEM_STAT_NEW_KERNEL("BFS END");
      }));
  PANDO_CHECK(wg.wait());
#endif
  PANDO_DRV_SET_STAGE_OTHER();

  if constexpr (COUNT_EDGE) {
    galois::doAll(
        toRead, +[](pando::Vector<typename G::VertexTopologyID>) {
          countEdges.printEdges();
          countEdges.resetCount();
        });
  }
  wg.deinitialize();
  return pando::Status::Success;
}

}; // namespace bfs

#endif // PANDO_BFS_GALOIS_SSSP_HPP_
