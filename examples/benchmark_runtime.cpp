// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <fstream>
#include <utility>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/thread_local_vector.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/graphs/mirror_dist_local_csr.hpp>
#include <pando-lib-galois/import/ingest_rmat_el.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/drv_info.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n numVertices -f filePath" << std::endl;
  std::exit(EXIT_FAILURE);
}

void HBMainDLCSR(std::uint64_t numVertices, pando::Array<char>&& filename) {
  using VT = std::uint64_t;
  using ET = std::uint64_t;
  using Graph = galois::DistLocalCSR<VT, ET>;

  Graph graph = galois::initializeELDLCSR<Graph, VT, ET>(filename, numVertices);
  filename.deinitialize();

  galois::WaitGroup wg{};
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();

  galois::HostLocalStorage<bool> temp;
  PANDO_CHECK(temp.initialize());
  /*
    PANDO_CHECK(galois::doAll(
        wgh, temp, +[](bool) {
          PANDO_MEM_STAT_NEW_KERNEL("BENCHMARK START");
        }));
    PANDO_CHECK(wg.wait());
  */
  PANDO_CHECK(galois::doAll(
      wgh, graph.vertexDataRange(), +[](pando::GlobalRef<typename Graph::VertexData> ref) {
        ref = static_cast<std::uint64_t>(0);
      }));
  PANDO_CHECK(wg.wait());
  /*
    PANDO_CHECK(galois::doAll(
        wgh, temp, +[](bool) {
          PANDO_MEM_STAT_NEW_KERNEL("BENCHMARK END");
        }));
    PANDO_CHECK(wg.wait());
  */
}

int pandoMain(int argc, char** argv) {
  auto place = pando::getCurrentPlace();

  if (place.node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
    std::uint64_t numVertices = 0;
    char* filePath = nullptr;

    // Other libraries may have called getopt before, so we reset optind for correctness
    optind = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:f:")) != -1) {
      switch (opt) {
        case 'n':
          numVertices = strtoull(optarg, nullptr, 10);
          break;
        case 'f':
          filePath = optarg;
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

    std::uint64_t size = strlen(filePath) + 1;
    pando::Array<char> filename;
    PANDO_CHECK(filename.initialize(size));
    for (std::uint64_t i = 0; i < size; i++) {
      filename[i] = filePath[i];
    }

    HBMainDLCSR(numVertices, std::move(filename));
  }
  pando::waitAll();
  return 0;
}
