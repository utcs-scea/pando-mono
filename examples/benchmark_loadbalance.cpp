
// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>

/**
 * @file benchmark_loadbalance.cpp
 * This benchmarks a very simple amount of work to see how it is load balanced in the system
 */

template <typename T>
using G = pando::GlobalPtr<T>;

template <typename T>
using Arr = pando::Array<T>;

template <typename T>
using Ref = pando::GlobalRef<T>;

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -c <numItemsPerChunk> " << std::endl;
  std::exit(EXIT_FAILURE);
}

enum POINTER : std::uint64_t { REGULAR = 0x1 << 0, GLOBALPTR = 0x1 << 1, CACHEPTR = 0x1 << 2 };

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  if (thisPlace.node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
    optind = 0;

    std::uint64_t numItemsPerChunk = 0;

    int opt;
    while ((opt = getopt(argc, argv, "c:")) != -1) {
      switch (opt) {
        case 'c':
          numItemsPerChunk = strtoull(optarg, nullptr, 10);
          break;
        default:
          printUsageExit(argv[0]);
      }
    }
    // Checks
    if (numItemsPerChunk != 1) {
      printUsageExit(argv[0]);
    }
    Arr<std::uint64_t> arr;
    PANDO_CHECK(arr.initialize(galois::getNumThreads()));
    galois::WaitGroup wg;
    PANDO_CHECK(wg.initialize(0));
    auto wgh = wg.getHandle();
    counter::HighResolutionCount<true> makeSpanCounter;
    makeSpanCounter.start();
    galois::doAllEvenlyPartition(
        wgh, arr, arr.size(),
        +[](Arr<std::uint64_t> arr, std::uint64_t i, std::uint64_t numThreads) {
          UNUSED(arr);
          UNUSED(i);
          UNUSED(numThreads);
        });
    PANDO_CHECK(wg.wait());
    auto time = makeSpanCounter.stop();
    arr.deinitialize();
    wg.deinitialize();
    std::cout << "The makespan of one task per thread was " << time.count() << std::endl;
  }
  pando::endExecution();
  return 0;
}
