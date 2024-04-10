// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <fstream>
#include <utility>

#include <pando-bfs-galois/sssp.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n numVertices -s srcVertex0 [-s srcVertex1] -f filePath"
            << std::endl;
  std::exit(EXIT_FAILURE);
}

void HBMainDLCSR(pando::Vector<std::uint64_t> srcVertices, std::uint64_t numVertices,
                 pando::Array<char>&& filename) {
#ifdef PRINTS
  std::cerr << "Construct Graph Begin" << std::endl;
#endif

  using VT = std::uint64_t;
  using ET = std::uint64_t;
  using Graph = galois::DistLocalCSR<VT, ET>;

  Graph graph = galois::initializeELDLCSR<Graph, VT, ET>(filename, numVertices);
  filename.deinitialize();

#ifdef PRINTS
  std::cerr << "Construct Graph End" << std::endl;
#endif

  using VertexTopologyID = typename galois::graph_traits<Graph>::VertexTopologyID;
  galois::HostLocalStorage<pando::Vector<VertexTopologyID>> phbfs{};
  PANDO_CHECK(phbfs.initialize());

  PANDO_CHECK(galois::doAll(
      phbfs, +[](pando::GlobalRef<pando::Vector<VertexTopologyID>> vecRef) {
        PANDO_CHECK(fmap(vecRef, initialize, 2));
        liftVoid(vecRef, clear);
      }));
  galois::PerThreadVector<VertexTopologyID> next;

  PANDO_CHECK(next.initialize());

  // Run BFS
  for (std::uint64_t srcVertex : srcVertices) {
    std::cout << "Source Vertex is " << srcVertex << std::endl;

    PANDO_CHECK(galois::SSSP(graph, srcVertex, next, phbfs));

    // Print Result
    for (std::uint64_t i = 0; i < numVertices; i++) {
      std::uint64_t val = graph.getData(graph.getTopologyID(i));
      std::cout << val << std::endl;
    }
  }
}
/*
void HBMainMDLCSR(pando::Vector<std::uint64_t> srcVertices, std::uint64_t numVertices,
                  pando::Array<char>&& filename) {
#ifdef PRINTS
  std::cerr << "Construct Graph Begin" << std::endl;
#endif

  using VT = std::uint64_t;
  using ET = std::uint64_t;
  using Graph = galois::MirrorDistLocalCSR<VT, ET>;

  galois::HostLocalStorageHeap::HeapInit();
   Graph graph = galois::initializeELDLCSR<Graph, VT, ET>(filename, numVertices);
  filename.deinitialize();

#ifdef PRINTS
  std::cerr << "Construct Graph End" << std::endl;
#endif

  using VertexTopologyID = typename galois::graph_traits<Graph>::VertexTopologyID;
  galois::HostLocalStorage<pando::Vector<VertexTopologyID>> phbfs{};
  PANDO_CHECK(phbfs.initialize());

  PANDO_CHECK(galois::doAll(
      phbfs, +[](pando::GlobalRef<pando::Vector<VertexTopologyID>> vecRef) {
        PANDO_CHECK(fmap(vecRef, initialize, 2));
        liftVoid(vecRef, clear);
      }));
  galois::PerThreadVector<VertexTopologyID> next;

  PANDO_CHECK(next.initialize());

  // Run BFS
  for (std::uint64_t srcVertex : srcVertices) {
    std::cout << "Source Vertex is " << srcVertex << std::endl;

    PANDO_CHECK(galois::MirroredSSSP(graph, srcVertex, next, phbfs));

    // Print Result
    for (std::uint64_t i = 0; i < numVertices; i++) {
      std::uint64_t val = graph.getData(graph.getTopologyID(i));
      std::cout << val << std::endl;
    }
  }
}
*/
int pandoMain(int argc, char** argv) {
  auto place = pando::getCurrentPlace();
  if (place.node.id == 0) {
    enum GraphMode { DLCSR, MDLCSR } graphMode{MDLCSR};
    std::uint64_t numVertices = 0;
    std::uint64_t srcVertex = 0;
    pando::Vector<std::uint64_t> srcVertices;
    PANDO_CHECK(srcVertices.initialize(0));
    char* filePath = nullptr;

    // Other libraries may have called getopt before, so we reset optind for correctness
    optind = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:s:f:dm")) != -1) {
      switch (opt) {
        case 'm':
          graphMode = MDLCSR;
          break;
        case 'd':
          graphMode = DLCSR;
          break;
        case 'n':
          numVertices = strtoull(optarg, nullptr, 10);
          break;
        case 'f':
          filePath = optarg;
          break;
        case 's':
          srcVertex = strtoull(optarg, nullptr, 10);
          PANDO_CHECK(srcVertices.pushBack(srcVertex));
          break;
        default:
          printUsageExit(argv[0]);
      }
    }

    if (numVertices == 0) {
      std::cerr << "numVertices is 0" << std::endl;
      printUsageExit(argv[0]);
    }
    if (filePath == nullptr) {
      std::cerr << "FilePath is nullptr" << std::endl;
      printUsageExit(argv[0]);
    }
    if (srcVertices.size() == 0) {
      std::cerr << "srcVertices is empty" << std::endl;
      printUsageExit(argv[0]);
    }

    std::uint64_t size = strlen(filePath) + 1;
    pando::Array<char> filename;
    PANDO_CHECK(filename.initialize(size));
    for (std::uint64_t i = 0; i < size; i++) {
      filename[i] = filePath[i];
    }

    if (graphMode == DLCSR) {
      HBMainDLCSR(srcVertices, numVertices, std::move(filename));
    } else {
      // HBMainMDLCSR(srcVertices, numVertices, std::move(filename));
    }
  }
  pando::waitAll();
  return 0;
}
