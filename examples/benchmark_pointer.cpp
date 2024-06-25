
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
 * @file benchmark_pointer.cpp
 * This benchmarks various types of pointer dereferences
 */

std::uint64_t currentLocation = 100;
std::uint64_t nodeDims = 100;

template <typename T>
struct CacheRef {
  pando::GlobalPtr<T> globalPtr;
  std::uint64_t cacheLoc;
  T* cachePtr;

private:
  bool tryToCache() {
    if (cacheLoc == nodeDims) {
      std::uint64_t nodeIndex = pando::extractNodeIndex(globalPtr.address).id;
      if (currentLocation == nodeIndex) {
        cacheLoc = nodeIndex;
        cachePtr = globalPtr.operator->();
        return true;
      }
    }
    return false;
  }

public:
  operator T() const {
    if (cacheLoc == currentLocation || tryToCache()) {
      return *cachePtr;
    } else {
      return *globalPtr;
    }
  }

  CacheRef<T> operator=(const T& value) {
    if (cacheLoc == currentLocation || tryToCache()) {
      *cachePtr = value;
    } else {
      *globalPtr = value;
    }
    return this;
  }
  template <typename U>
  CacheRef<T> operator+=(const U& y) {
    if (cacheLoc == currentLocation || tryToCache()) {
      *cachePtr += y;
    } else {
      *globalPtr += y;
    }
    return *this;
  }
};

template <typename T>
struct CachePtr {
  CacheRef<T> ref;

  CacheRef<T> operator*() {
    return ref;
  }
};

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -n numAccesses [-r] [-g] [-c]" << std::endl;
  std::exit(EXIT_FAILURE);
}

enum POINTER : std::uint64_t { REGULAR = 0x1 << 0, GLOBALPTR = 0x1 << 1, CACHEPTR = 0x1 << 2 };

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  currentLocation = thisPlace.node.id;
  nodeDims = pando::getNodeDims().id;
  if (thisPlace.node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
    optind = 0;

    std::uint64_t numAccesses = 0;
    std::uint64_t ptrTypes = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:rgc")) != -1) {
      switch (opt) {
        case 'n':
          numAccesses = strtoull(optarg, nullptr, 10);
          break;
        case 'r':
          ptrTypes |= REGULAR;
          break;
        case 'g':
          ptrTypes |= GLOBALPTR;
          break;
        case 'c':
          ptrTypes |= CACHEPTR;
          break;
        default:
          printUsageExit(argv[0]);
      }
    }

    // Checks
    if (numAccesses == 0) {
      printUsageExit(argv[0]);
    }
    if (ptrTypes == 0) {
      printUsageExit(argv[0]);
    }

    // Initialize Regular
    std::uint64_t* ptr = (std::uint64_t*)malloc(sizeof(std::uint64_t));

    // Initialize GlobalPtr
    pando::GlobalPtr<std::uint64_t> gptr;
    pando::LocalStorageGuard<std::uint64_t> gptrGuard(gptr, 1);

    // Initialize CachePtr
    CachePtr<std::uint64_t> cptr;
    pando::LocalStorageGuard<std::uint64_t> cptrGuard(cptr.ref.globalPtr, 1);
    cptr.ref.cacheLoc = nodeDims;

    std::array<std::uint64_t, 16> simpleArr;
    for (std::uint64_t i = 0; i < simpleArr.size(); i++) {
      simpleArr[i] = i;
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> normalBegin, normalEnd;
    std::chrono::time_point<std::chrono::high_resolution_clock> gptrBegin, gptrEnd;
    std::chrono::time_point<std::chrono::high_resolution_clock> cptrBegin, cptrEnd;
    if (ptrTypes & REGULAR) {
      *ptr = 0;
      normalBegin = std::chrono::high_resolution_clock::now();
      for (std::uint64_t i = 0; i < numAccesses; i++) {
        *ptr += simpleArr[i % simpleArr.size()];
      }
      normalEnd = std::chrono::high_resolution_clock::now();
    }

    if (ptrTypes & GLOBALPTR) {
      *gptr = 0;
      gptrBegin = std::chrono::high_resolution_clock::now();
      for (std::uint64_t i = 0; i < numAccesses; i++) {
        *gptr += simpleArr[i % simpleArr.size()];
      }
      gptrEnd = std::chrono::high_resolution_clock::now();
    }

    if (ptrTypes & CACHEPTR) {
      *gptr = 0;
      cptrBegin = std::chrono::high_resolution_clock::now();
      for (std::uint64_t i = 0; i < numAccesses; i++) {
        *cptr += simpleArr[i % simpleArr.size()];
      }
      cptrEnd = std::chrono::high_resolution_clock::now();
    }

    if (ptrTypes & REGULAR) {
      std::cout << "Normal Pointer took: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(normalEnd - normalBegin)
                << "\n";
    }
    if (ptrTypes & GLOBALPTR) {
      std::cout << "Global Pointer took: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(gptrEnd - gptrBegin)
                << "\n";
    }
    if (ptrTypes & CACHEPTR) {
      std::cout << "Cache Pointer took: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(cptrEnd - cptrBegin)
                << "\n";
    }
    std::cout << std::flush;
  }
  pando::endExecution();
  return 0;
}
