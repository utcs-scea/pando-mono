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

void TestFunc(galois::ELVertex mirror, pando::GlobalRef<galois::ELVertex> master) {
  fmapVoid(master, set, mirror.get() + 1);
}

void runTest(const char* elFile, std::uint64_t numVertices) {
  using ET = galois::ELEdge;
  using VT = galois::ELVertex;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;
  pando::Array<char> filename;
  std::size_t length = strlen(elFile);
  PANDO_CHECK(filename.initialize(length + 1));
  for (std::size_t i = 0; i < length; i++) {
    filename[i] = elFile[i];
  }
  filename[length] = '\0'; // Ensure the string is null-terminated

  if (pando::getCurrentPlace().node.id == 0) {
    Graph graph =
        galois::initializeELDLCSR<Graph, galois::ELVertex, galois::ELEdge>(filename, numVertices);

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
    for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
      PANDO_CHECK(
          pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                           func, barrier, graph.getMirrorBitSets()));
    }
    PANDO_CHECK(barrier.wait());

    for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
      pando::GlobalRef<Graph::LocalVertexRange> mirrorRange = graph.getMirrorRange(nodeId);
      for (Graph::VertexTopologyID mirrorTopologyID = *lift(mirrorRange, begin);
           mirrorTopologyID < *lift(mirrorRange, end); mirrorTopologyID++) {
        pando::GlobalRef<Graph::VertexData> mirrorData = graph.getData(mirrorTopologyID);
        fmapVoid(mirrorData, set, lift(mirrorData, get) + 1);
      }
    }

    graph.sync(TestFunc);

    for (std::int16_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
      pando::GlobalRef<pando::Array<bool>> mirrorBitSet = graph.getMirrorBitSet(nodeId);
      pando::GlobalRef<pando::Array<Graph::MirrorToMasterMap>> localMirrorToRemoteMasterOrderedMap =
          graph.getLocalMirrorToRemoteMasterOrderedMap(nodeId);
      for (std::uint64_t i = 0ul; i < lift(mirrorBitSet, size); i++) {
        Graph::MirrorToMasterMap m = fmap(localMirrorToRemoteMasterOrderedMap, get, i);
        Graph::VertexTopologyID mirrorTopologyID = m.getMirror();
        Graph::VertexTokenID mirrorTokenID = graph.getTokenID(mirrorTopologyID);
        Graph::VertexData mirrorData = graph.getData(mirrorTopologyID);
        std::cout << "(Mirror) Host " << nodeId << " LocalMirrorTokenID: " << mirrorTokenID
                  << " MirrorData: " << mirrorData.id << std::endl;
      }
    }

    graph.deinitialize();
  }
  pando::waitAll();
}
