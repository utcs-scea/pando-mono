// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/approx_match.h"
#include "pando-wf2-galois/export.h"
#include "pando-wf2-galois/import_wmd.hpp"

void printUsageExit(char* argv0) {
  std::printf(
      "Usage: %s "
      "-k <num-matches> "
      "-d <data-file-path> "
      "-p <pattern-file-path> "
      "\n",
      argv0);
}

int pandoMain(int argc, char** argv) {
  if (pando::getCurrentPlace().node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
  }
  uint64_t k = 1;
  const char* data_file = nullptr;
  const char* pattern_file = nullptr;

  // Other libraries may have called getopt before, so we reset optind for correctness
  optind = 0;

  char opt;
  while ((opt = getopt(argc, argv, "k:d:p:")) != -1) {
    switch (opt) {
      case 'k':
        k = strtoull(optarg, nullptr, 10);
        break;
      case 'd':
        data_file = optarg;
        break;
      case 'p':
        pattern_file = optarg;
        break;
      default:
        printUsageExit(argv[0]);
        std::cout << opt << std::endl;
        PANDO_ABORT("invalid arguments");
    }
  }
  if (data_file == nullptr || pattern_file == nullptr) {
    printUsageExit(argv[0]);
    PANDO_ABORT("Invalid Arguments");
  }

  auto place = pando::getCurrentPlace();

  // Import Graph
  if (place.node.id == 0) {
    std::string pattern_filename(pattern_file);
    pando::GlobalPtr<wf2::approx::Graph> lhs_ptr = wf::ImportWMDGraph(pattern_filename);
    std::string data_filename(data_file);
    pando::GlobalPtr<wf2::approx::Graph> rhs_ptr = wf::ImportWMDGraph(data_filename);

    wf2::approx::match(lhs_ptr, rhs_ptr, k);
  }

  pando::waitAll();
  return 0;
}
