// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>

#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/global_barrier.hpp>
#include <pando-lib-galois/sync/simple_lock.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

#include <pando-lib-galois/graphs/dist_array_csr.hpp>

using VertexTokenID = std::uint64_t;

struct MirrorToMasterMap {
  MirrorToMasterMap() = default;
  MirrorToMasterMap(VertexTokenID _mirror, VertexTokenID _master)
      : mirror(_mirror), master(_master) {}
  VertexTokenID mirror;
  VertexTokenID master;
  VertexTokenID getMirror() {
    return mirror;
  }
  VertexTokenID getMaster() {
    return master;
  }
};

TEST(MapExchange, Simple) {
  auto dims = pando::getPlaceDims();

  galois::GlobalBarrier barrier1, barrier2;
  EXPECT_EQ(barrier1.initialize(dims.node.id), pando::Status::Success);
  EXPECT_EQ(barrier2.initialize(dims.node.id), pando::Status::Success);

  galois::HostLocalStorage<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedTable;
  EXPECT_EQ(localMirrorToRemoteMasterOrderedTable.initialize(), pando::Status::Success);

  auto func = +[](galois::GlobalBarrier barrier1,
                  galois::HostLocalStorage<pando::Array<MirrorToMasterMap>>
                      localMirrorToRemoteMasterOrderedTable,
                  std::int16_t nodeId) {
    pando::GlobalRef<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterMap =
        localMirrorToRemoteMasterOrderedTable[nodeId];
    EXPECT_EQ(fmap(localMirrorToRemoteMasterMap, initialize, 2 * pando::getPlaceDims().node.id),
              pando::Status::Success);
    for (std::int16_t i = 0; i < 2 * pando::getPlaceDims().node.id; i++) {
      VertexTokenID vertexId = nodeId * 2 * pando::getPlaceDims().node.id + i;
      fmap(localMirrorToRemoteMasterMap, get, i) = MirrorToMasterMap{vertexId, vertexId};
    }
    barrier1.done();
  };
  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    EXPECT_EQ(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                         func, barrier1, localMirrorToRemoteMasterOrderedTable, nodeId),
        pando::Status::Success);
  }
  EXPECT_EQ(barrier1.wait(), pando::Status::Success);

  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    std::cout << "Host " << nodeId << " Map (Before):" << std::endl;
    pando::Array<MirrorToMasterMap> tempMap = localMirrorToRemoteMasterOrderedTable[nodeId];
    for (std::int16_t i = 0; i < 2 * dims.node.id; i++) {
      MirrorToMasterMap m = tempMap[i];
      std::cout << m.getMirror() << ", " << m.getMaster() << std::endl;
    }
  }

  galois::HostLocalStorage<pando::Vector<pando::Vector<MirrorToMasterMap>>>
      localMasterToRemoteMirrorTable;
  EXPECT_EQ(localMasterToRemoteMirrorTable.initialize(), pando::Status::Success);

  for (std::int16_t i = 0; i < dims.node.id; i++) {
    pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>> localMasterToRemoteMirrorMap =
        localMasterToRemoteMirrorTable[i];
    EXPECT_EQ(fmap(localMasterToRemoteMirrorMap, initialize, dims.node.id), pando::Status::Success);
    for (std::int16_t i = 0; i < dims.node.id; i++) {
      pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
          fmap(localMasterToRemoteMirrorMap, get, i);
      EXPECT_EQ(fmap(mapVectorFromHost, initialize, 0), pando::Status::Success);
    }
  }

  auto state = galois::make_tpl(barrier2, localMasterToRemoteMirrorTable);

  galois::doAll(
      state, localMirrorToRemoteMasterOrderedTable,
      +[](decltype(state) state,
          pando::GlobalRef<pando::Array<MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap) {
        auto [barrier2, localMasterToRemoteMirrorTable] = state;
        for (std::uint64_t i = 0ul; i < lift(localMirrorToRemoteMasterOrderedMap, size); i++) {
          MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, get, i);
          VertexTokenID masterTokenID = m.getMaster();
          std::uint64_t physicalHost = masterTokenID % pando::getPlaceDims().node.id;

          std::cout << "tokenID: " << m.master << ", physicalHost: " << physicalHost << std::endl;

          pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>>
              localMasterToRemoteMirrorMap = localMasterToRemoteMirrorTable[physicalHost];
          pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
              fmap(localMasterToRemoteMirrorMap, get, pando::getCurrentPlace().node.id);

          EXPECT_EQ(fmap(mapVectorFromHost, pushBack, m), pando::Status::Success);
        }

        barrier2.done();
        EXPECT_EQ(barrier2.wait(), pando::Status::Success);
      });

  for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    std::cout << "Host " << nodeId << " Map (After):" << std::endl;
    pando::GlobalRef<pando::Vector<pando::Vector<MirrorToMasterMap>>> localMasterToRemoteMirrorMap =
        localMasterToRemoteMirrorTable[nodeId];
    for (std::int16_t i = 0; i < dims.node.id; i++) {
      pando::GlobalRef<pando::Vector<MirrorToMasterMap>> mapVectorFromHost =
          fmap(localMasterToRemoteMirrorMap, get, i);
      std::cout << "from host " << i << " :";
      for (std::uint64_t j = 0; j < lift(mapVectorFromHost, size); j++) {
        MirrorToMasterMap m = fmap(mapVectorFromHost, get, j);
        std::cout << "[" << m.getMirror() << ", " << m.getMaster() << "]";
      }
      std::cout << std::endl;
    }
  }

  barrier1.deinitialize();
  barrier2.deinitialize();
}
