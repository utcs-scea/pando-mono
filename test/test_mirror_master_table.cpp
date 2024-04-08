
// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <cstdint>
#include <iostream>
#include <numeric>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/memory_resource.hpp>
#include <pando-rt/pando-rt.hpp>

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
  galois::HostLocalStorageHeap::HeapInit();
  galois::PodLocalStorageHeap::HeapInit();
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
    // Iterate over vertices
    std::uint64_t vid = 0;
    auto mirror_master_array = graph.getLocalMirrorToRemoteMasterOrderedTable();
    for (auto elem : mirror_master_array) {
      std::cout << "SET, " << lift(elem, getMirror).address << ", " << lift(elem, getMaster).address
                << std::endl;
    }

    for (typename Graph::VertexTopologyID vert : graph.vertices()) {
      vid++;
      for (typename Graph::EdgeHandle eh : graph.edges(vert)) {
        typename Graph::VertexTokenID dstTok = graph.getTokenID(graph.getEdgeDst(eh));

        auto mirrorTopology = graph.getTopologyID(dstTok);
        auto masterTopology = graph.getGlobalTopologyID(dstTok);
        if (mirrorTopology != masterTopology) {
          // If global, and local have different value.
          // It means current one have mirror. Mirror is local, but master is not.
          std::cout << "TRUE, " << mirrorTopology.address << ", " << masterTopology.address
                    << std::endl;
        } else {
          // If I don't have mirror, that could be because it is in local, or never be a destination
          // from me.
          if (graph.isLocal(masterTopology)) {
            // If it is from me, it is in my master range.
            std::cout << "FALSE, " << mirrorTopology.address << ", " << masterTopology.address
                      << std::endl;
          }
        }
      }
    }
    graph.deinitialize();
  }
  pando::waitAll();
}
