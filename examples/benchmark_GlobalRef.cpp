// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include <pando-rt/export.h>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/pando-rt.hpp>

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n arraySize [-g] [-c]" << std::endl;
  std::exit(EXIT_FAILURE);
}

enum METHOD : std::uint64_t { GLOBAL = 0x1 << 0, COPY = 0x1 << 1 };

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  if (thisPlace.node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();

    optind = 0;

    std::uint64_t arraySize = 0;
    std::uint64_t methodTypes = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:gc")) != -1) {
      switch (opt) {
        case 'n':
          arraySize = strtoull(optarg, nullptr, 10);
          break;
        case 'g':
          methodTypes |= GLOBAL;
          break;
        case 'c':
          methodTypes |= COPY;
          break;
        default:
          printUsageExit(argv[0]);
      }
    }

    // Checks
    if (arraySize == 0) {
      printUsageExit(argv[0]);
    }
    if (methodTypes == 0) {
      printUsageExit(argv[0]);
    }

    galois::HostLocalStorage<pando::Array<std::int64_t>> hostVecs;
    PANDO_CHECK(hostVecs.initialize());

    galois::WaitGroup wg;
    PANDO_CHECK(wg.initialize(0));
    auto wgh = wg.getHandle();

    PANDO_CHECK(galois::doAll(
        wgh, arraySize, hostVecs,
        +[](std::uint64_t arraySize, pando::GlobalRef<pando::Array<std::int64_t>> hostVec) {
          PANDO_CHECK(fmap(hostVec, initialize, arraySize));
          fmapVoid(hostVec, fill, 0);
        }));
    PANDO_CHECK(wg.wait());

    std::cout << "value = " << fmap(hostVecs[0], operator[], 0) << "\n";

    std::chrono::time_point<std::chrono::high_resolution_clock> globalBegin, globalEnd;
    std::chrono::time_point<std::chrono::high_resolution_clock> copyBegin, copyEnd;

    if (methodTypes & GLOBAL) {
      globalBegin = std::chrono::high_resolution_clock::now();
      PANDO_CHECK(galois::doAll(
          wgh, arraySize, hostVecs,
          +[](std::uint64_t arraySize, pando::GlobalRef<pando::Array<std::int64_t>> hostVec) {
            for (std::uint64_t i = 0; i < arraySize; i++) {
              fmap(hostVec, operator[], i) = i + 1;
            }
          }));
      PANDO_CHECK(wg.wait());
      globalEnd = std::chrono::high_resolution_clock::now();

      std::cout << "value = " << fmap(hostVecs[0], operator[], 0) << "\n";

      PANDO_CHECK(galois::doAll(
          wgh, arraySize, hostVecs,
          +[](std::uint64_t arraySize, pando::GlobalRef<pando::Array<std::int64_t>> hostVec) {
            for (std::uint64_t i = 0; i < arraySize; i++) {
              fmap(hostVec, operator[], i) = 0;
            }
          }));
      PANDO_CHECK(wg.wait());

      std::cout << "value = " << fmap(hostVecs[0], operator[], 0) << "\n";
    }

    if (methodTypes & COPY) {
      copyBegin = std::chrono::high_resolution_clock::now();
      PANDO_CHECK(galois::doAll(
          wgh, arraySize, hostVecs,
          +[](std::uint64_t arraySize, pando::GlobalRef<pando::Array<std::int64_t>> hostVec) {
            pando::Array<std::int64_t> temp = hostVec;
            for (std::uint64_t i = 0; i < arraySize; i++) {
              temp[i] = i + 1;
            }
            hostVec = temp;
          }));
      PANDO_CHECK(wg.wait());
      copyEnd = std::chrono::high_resolution_clock::now();

      std::cout << "value = " << fmap(hostVecs[0], operator[], 0) << "\n";
    }

    if (methodTypes & GLOBAL) {
      std::cout << "GlobalRef took: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(globalEnd - globalBegin)
                << std::endl;
    }

    if (methodTypes & COPY) {
      std::cout << "Copy took: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(copyEnd - copyBegin)
                << std::endl;
    }

    std::cout << std::flush;
  }
  pando::waitAll();
  return 0;
}
