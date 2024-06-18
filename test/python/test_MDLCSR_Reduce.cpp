// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include <cstdint>
#include <iostream>
#include <numeric>
#include <variant>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/global_barrier.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n numVertices -f filepath" << std::endl;
  std::exit(EXIT_FAILURE);
}

template <typename T>
using GV = pando::GlobalPtr<pando::Vector<T>>;

template <typename T>
using V = pando::Vector<T>;

template <typename T>
using G = pando::GlobalPtr<T>;
void runTest(const char* elFile, std::uint64_t numVertices);

int pandoMain(int argc, char** argv) {
  if (pando::getCurrentPlace().node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
  }
  std::uint64_t numVertices = 0;
  char* filepath = nullptr;
  optind = 0;
  int opt;

  while ((opt = getopt(argc, argv, "n:f:")) != -1) {
    switch (opt) {
      case 'n':
        numVertices = strtoull(optarg, nullptr, 10);
        break;
      case 'f':
        filepath = optarg;
        break;
      default:
        printUsageExit(argv[0]);
    }
  }
  if (numVertices == 0) {
    printUsageExit(argv[0]);
  }
  if (filepath == nullptr) {
    printUsageExit(argv[0]);
  }
  runTest(filepath, numVertices);
  return 0;
}

template <typename Graph>
void TestFunc(typename Graph::VertexData mirrorData,
              pando::GlobalRef<typename Graph::VertexData> masterData) {
  masterData = mirrorData + 1;
}

void runTest(const char* elFile, std::uint64_t numVertices) {
  using VT = std::uint64_t;
  using ET = std::uint64_t;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;
  pando::Array<char> filename;
  std::size_t length = strlen(elFile);
  PANDO_CHECK(filename.initialize(length + 1));
  for (std::size_t i = 0; i < length; i++) {
    filename[i] = elFile[i];
  }
  filename[length] = '\0'; // Ensure the string is null-terminated

  if (pando::getCurrentPlace().node.id == 0) {
    Graph graph = galois::initializeELDLCSR<Graph, VT, ET>(filename, numVertices);

    auto dims = pando::getPlaceDims();

    galois::GlobalBarrier barrier;
    PANDO_CHECK(barrier.initialize(dims.node.id));

    auto func = +[](galois::GlobalBarrier barrier,
                    galois::HostLocalStorage<pando::Array<bool>> mirrorBitSets) {
      pando::GlobalRef<pando::Array<bool>> mirrorBitSet =
          mirrorBitSets[pando::getCurrentPlace().node.id];
      fmapVoid(mirrorBitSet, fill, true);
      barrier.done();
    };
    for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
      PANDO_CHECK(
          pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                           func, barrier, graph.getMirrorBitSets()));
    }
    PANDO_CHECK(barrier.wait());

    graph.reduce(TestFunc<Graph>);

    for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
      pando::GlobalRef<pando::Array<bool>> mirrorBitSet = graph.getMirrorBitSet(nodeId);
      pando::GlobalRef<pando::Array<Graph::MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap =
          graph.getLocalMirrorToRemoteMasterOrderedMap(nodeId);
      for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
        Graph::MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, operator[], i);
        Graph::VertexTopologyID mirrorTopologyID = m.getMirror();
        Graph::VertexTopologyID masterTopologyID = m.getMaster();
        Graph::VertexTokenID masterTokenID = graph.getTokenID(masterTopologyID);
        std::uint64_t masterHost = graph.getPhysicalHostID(masterTokenID);
        std::cout << "(Mirror) Host " << nodeId
                  << " LocalMirrorTopologyID: " << mirrorTopologyID.address
                  << " RemoteMasterTopologyID: " << masterTopologyID.address
                  << " RemoteMasterHost: " << masterHost << std::endl;
      }

      pando::GlobalRef<pando::Array<bool>> masterBitSet = graph.getMasterBitSet(nodeId);
      pando::GlobalRef<Graph::LocalVertexRange> masterRange = graph.getMasterRange(nodeId);
      for (std::uint64_t i = 0ul; i < lift(masterBitSet, size); i++) {
        bool bit = fmap(masterBitSet, operator[], i);
        Graph::VertexTopologyID masterTopologyID = *lift(masterRange, begin) + i;
        Graph::VertexTokenID masterTokenID = graph.getTokenID(masterTopologyID);
        Graph::VertexData masterData = graph.getData(masterTopologyID);
        std::cout << "(Master) Host " << nodeId
                  << " LocalMasterTopologyID: " << masterTopologyID.address
                  << " LocalMasterTokenID: " << masterTokenID << " MasterData: " << masterData
                  << " Bit: " << bit << std::endl;
      }
    }

    graph.deinitialize();
  }
  pando::endExecution();
}
