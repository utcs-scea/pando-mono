
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
    Arr<G<std::uint64_t>> arr;
    PANDO_CHECK(arr.initialize(galois::getNumThreads()));
    G<std::uint64_t> hello;
    pando::LocalStorageGuard<std::uint64_t> helloGuard(hello, 1);
    *hello = 0;
    for (Ref<G<std::uint64_t>> item : arr) {
      item = hello;
    }
    galois::doAll(
        arr, +[](G<std::uint64_t> hello) {
          pando::atomicFetchAdd(hello, 1, std::memory_order_relaxed);
          while (pando::atomicLoad(hello, std::memory_order_relaxed) != galois::getNumThreads()) {}
        });
    arr.deinitialize();
  }
  pando::waitAll();
  return 0;
}
