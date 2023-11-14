// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <cstdint>
#include <iostream>

#include <pando-lib-galois/graphs/edge_list_importer.hpp>
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

int pandoMain(int argc, char** argv) {
  std::uint64_t numVertices = 0;
  char* filepath = nullptr;
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

  if (pando::getCurrentPlace().node.id == 0) {
    GV<V<std::uint64_t>> listcsrPtr = static_cast<GV<V<std::uint64_t>>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(V<V<std::uint64_t>>)));

    PANDO_CHECK(galois::importELFile(numVertices, filepath, *listcsrPtr));
    pando::Vector<pando::Vector<std::uint64_t>> listcsr = *listcsrPtr;
    for (pando::Vector<std::uint64_t> vec : listcsr) {
      std::vector<std::uint64_t> stdvec;
      for (std::uint64_t val : vec) {
        stdvec.push_back(val);
      }
      std::sort(stdvec.begin(), stdvec.end());
      for (std::uint64_t val : stdvec) {
        std::cout << val << ' ';
      }
      std::cout << std::endl;
    }
  }
  return 0;
}
