// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <chrono>
#include <memory>

#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>
#include <tc_naive.hpp>

template <typename T>
using G = pando::GlobalPtr<T>;
template <typename T>
using R = pando::GlobalRef<T>;
template <typename T>
using A = pando::Array<T>;
template <typename T>
using V = pando::Vector<T>;
template <typename K, typename V>
using H = galois::HashTable<K, V>;

void HBMain(pando::Notification::HandleType hb_done, pando::Array<char> filename,
            uint64_t num_vertices) {
#if BENCHMARK
  auto time_graph_import_st = std::chrono::high_resolution_clock().now();
#endif

  GraphDA graph =
      galois::initializeELDACSR<galois::ELVertex, galois::ELEdge>(filename, num_vertices);

#if BENCHMARK
  auto time_graph_import_end = std::chrono::high_resolution_clock().now();
  std::cout << "Time_Graph_Creation(ms), "
            << std::chrono::duration_cast<std::chrono::milliseconds>(time_graph_import_end -
                                                                     time_graph_import_st)
                   .count()
            << "\n";
#endif

  pando::GlobalPtr<GraphDA> graph_ptr = static_cast<pando::GlobalPtr<GraphDA>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDA)));
  *graph_ptr = graph;

  // Run TC
  G<std::uint64_t> countPtr = static_cast<pando::GlobalPtr<uint64_t>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(uint64_t)));

  if (countPtr == nullptr) {
    PANDO_ABORT("Unable to allocate memory\n");
  }
  *countPtr = 0;
#if BENCHMARK
  auto TCStart = std::chrono::high_resolution_clock().now();
#endif
  PANDO_CHECK(galois::DirOptNaiveTC(graph_ptr, countPtr));
#if BENCHMARK
  auto TCEnd = std::chrono::high_resolution_clock().now();
#endif

#if BENCHMARK
  std::cerr << "TC Time (ns):\t"
            << std::chrono::duration_cast<std::chrono::nanoseconds>(TCEnd - TCStart).count()
            << std::endl;
#endif

  // Print Result
  std::cout << static_cast<std::uint64_t>(*countPtr) << std::endl;
  hb_done.notify();
}

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  std::shared_ptr<CommandLineOptions> opts = read_cmd_line_args(argc, argv);
  if (thisPlace.node.id == 0) {
    pando::Array<char> filename;
    PANDO_CHECK(filename.initialize(opts->elFile.size()));
    for (uint64_t i = 0; i < opts->elFile.size(); i++)
      filename[i] = opts->elFile[i];

    pando::Notification necessary;
    PANDO_CHECK(necessary.init());
    PANDO_CHECK(pando::executeOn(pando::anyPlace, &HBMain, necessary.getHandle(), filename,
                                 opts->num_vertices));
    necessary.wait();
    filename.deinitialize();
  }
  pando::waitAll();
  return 0;
}
