// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <tc_algos.hpp>

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  std::shared_ptr<CommandLineOptions> opts = read_cmd_line_args(argc, argv);

  if (thisPlace.node.id == COORDINATOR_ID) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
#if BENCHMARK
    auto time_e2e_st = std::chrono::high_resolution_clock().now();
#endif

    pando::Array<char> filename;
    PANDO_CHECK(filename.initialize(opts->elFile.size()));
    for (uint64_t i = 0; i < opts->elFile.size(); i++)
      filename[i] = opts->elFile[i];

    galois::DAccumulator<uint64_t> final_tri_count{};
    PANDO_CHECK(final_tri_count.initialize());

    pando::Notification necessary;
    PANDO_CHECK(necessary.init());
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &HBMainTC, necessary.getHandle(), filename, opts->num_vertices,
                                 opts->load_balanced_graph, opts->tc_chunk, final_tri_count));
    necessary.wait();
    filename.deinitialize();
    std::cout << "*** FINAL TRI COUNT = " << final_tri_count.reduce() << "\n";

#if BENCHMARK
    auto time_e2e_end = std::chrono::high_resolution_clock().now();
    std::cout
        << "Time_E2E(ns), "
        << std::chrono::duration_cast<std::chrono::nanoseconds>(time_e2e_end - time_e2e_st).count()
        << "\n";
#endif
  }
  pando::waitAll();
  return 0;
}
