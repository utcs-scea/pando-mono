
// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

/**
 * @file benchmark_pointer.cpp
 * This benchmarks various types of pointer dereferences
 */

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n numAccesses" << std::endl;
  std::exit(EXIT_FAILURE);
}

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  if (thisPlace.node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
    optind = 0;

    std::uint64_t numAccesses = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:s:f:dm")) != -1) {
      switch (opt) {
        case 'n':
          numAccesses = strtoull(optarg, nullptr, 10);
          break;
        default:
          printUsageExit(argv[0]);
      }
    }

    std::uint64_t* ptr = (std::uint64_t*)malloc(sizeof(std::uint64_t));
    pando::GlobalPtr<std::uint64_t> gptr;
    pando::LocalStorageGuard<std::uint64_t> gptrGuard(gptr, 1);

    std::array<std::uint64_t, 16> simpleArr;
    for (std::uint64_t i = 0; i < simpleArr.size(); i++) {
      simpleArr[i] = i;
    }

    *ptr = 0;
    auto normalBegin = std::chrono::high_resolution_clock::now();
    for (std::uint64_t i = 0; i < numAccesses; i++) {
      *ptr += simpleArr[i % simpleArr.size()];
    }
    auto normalEnd = std::chrono::high_resolution_clock::now();
    *gptr = 0;
    auto gptrBegin = std::chrono::high_resolution_clock::now();
    for (std::uint64_t i = 0; i < numAccesses; i++) {
      *gptr += simpleArr[i % simpleArr.size()];
    }

    auto gptrEnd = std::chrono::high_resolution_clock::now();

    std::cout << "Normal Pointer took: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(normalEnd - normalBegin)
              << "\n Global Pointer took: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(gptrEnd - gptrBegin)
              << std::endl;
  }
  pando::waitAll();
  return 0;
}
