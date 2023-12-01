// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_DIST_ACCUMULATOR_HPP_
#define PANDO_LIB_GALOIS_UTILITY_DIST_ACCUMULATOR_HPP_

#include <pando-rt/export.h>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/atomic.hpp>
#include <pando-rt/sync/notification.hpp>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

namespace galois {

/**
 * @brief This is a basic mechanism for computing distributed atomic values using add and subtract
 * operations
 */
template <typename T>
class DAccumulator {
  ///@brief This is a distributed array of the counters used by each PXN
  galois::DistArray<T> localCounters;
  ///@brief This is a pointer to the computed global value, populated by reduce()
  pando::GlobalPtr<T> globalValue;
  ///@brief Tracks whether global_value holds a valid value
  bool globalValueComputed = false;

public:
  DAccumulator() noexcept = default;

  /**
   * @brief initializes the DAccumulator
   *
   * @param[in] place        the location the counter should be allocated at
   * @param[in] memoryType   the type of memory the waitgroup should be allocated in
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] pando::Status initialize(pando::Place place, pando::MemoryType memoryType) {
    pando::Status err;
    int16_t pxns = pando::getPlaceDims().node.id;

    auto expect = pando::allocateMemory<T>(1, place, memoryType);
    if (!expect.hasValue()) {
      return expect.error();
    }
    globalValue = expect.value();

    pando::Vector<galois::PlaceType> vec;
    err = vec.initialize(pxns);
    if (err != pando::Status::Success) {
      pando::deallocateMemory<T>(globalValue, 1);
      return err;
    }

    for (std::int16_t i = 0; i < pxns; i++) {
      vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                         pando::MemoryType::Main};
    }

    err = localCounters.initialize(vec.begin(), vec.end(), pxns);
    vec.deinitialize();
    if (err != pando::Status::Success) {
      pando::deallocateMemory<T>(globalValue, 1);
      return err;
    }
    reset();
    return err;
  }

  /**
   * @brief initializes the DAccumulator
   *
   * @warning one of the initialize methods must be called before use
   */
  [[nodiscard]] pando::Status initialize() {
    return initialize(pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  /**
   * @brief deinitializes the waitgroup and frees associated memory
   *
   * @warning not threadsafe but designed to be idempotent.
   */
  void deinitialize() {
    localCounters.deinitialize();
    pando::deallocateMemory<T>(globalValue, 1);
  }

  /**
   * @brief resets all local counters to 0 and sets globalValueComputed to false
   */
  void reset() {
    for (pando::GlobalRef<T> localCounter : localCounters) {
      localCounter = 0;
    }
    globalValueComputed = false;
    *globalValue = 0;
    pando::atomicThreadFence(std::memory_order_release);
  }

  /**
   * @brief reduce adds all local counters together, sets globalValueComputed, and returns the value
   *
   * @warning every time this is called the global value is recomputed, for subsequent access to the
   * globally computed value after the first reduce, use get instead
   */
  T reduce() {
    for (pando::GlobalRef<T> ref : localCounters) {
      T val = pando::atomicLoad(&ref, std::memory_order_relaxed);
      pando::atomicFetchAdd(globalValue, val, std::memory_order_release);
    }
    globalValueComputed = true;
    return *globalValue;
  }

  /**
   * @brief get returns the cached global value from the last reduce call
   *
   * @warning if called before reduce or after a reset this will return 0
   */
  T get() {
    if (!globalValueComputed) {
      return 0;
    }
    return *globalValue;
  }

  /**
   * @brief add adds the given delta to the local accumulator
   */
  void add(T delta) {
    pando::atomicFetchAdd(&localCounters[pando::getCurrentPlace().node.id], delta,
                          std::memory_order_release);
  }
  /**
   * @brief increment adds 1 to the local accumulator
   */
  void increment() {
    add(static_cast<T>(1));
  }
  /**
   * @brief subtract subtracts the given delta from the local accumulator
   */
  void subtract(T delta) {
    pando::atomicFetchSub(&localCounters[pando::getCurrentPlace().node.id], delta,
                          std::memory_order_release);
  }
  /**
   * @brief decrement subtracts 1 from the local accumulator
   */
  void decrement() {
    subtract(static_cast<T>(1));
  }
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_UTILITY_DIST_ACCUMULATOR_HPP_
