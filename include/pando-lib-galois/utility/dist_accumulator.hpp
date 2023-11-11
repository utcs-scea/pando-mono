// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_DIST_ACCUMULATOR_HPP_
#define PANDO_LIB_GALOIS_UTILITY_DIST_ACCUMULATOR_HPP_

#include <pando-rt/export.h>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_utilities.hpp>
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
  galois::DistArray<T> globalValue;
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
    int16_t pxns = pando::getPlaceDims().node.id;
    pando::Status err;
    pando::Vector<galois::PlaceType> vec;
    err = vec.initialize(pxns);
    if (err != pando::Status::Success) {
      return err;
    }
    pando::Vector<galois::PlaceType> globalPlace;
    err = globalPlace.initialize(1);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      return err;
    }

    for (std::int16_t i = 0; i < pxns; i++) {
      vec[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                         pando::MemoryType::Main};
    }
    globalPlace[0] = PlaceType{place, memoryType};

    err = localCounters.initialize(vec.begin(), vec.end(), pxns);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      globalPlace.deinitialize();
      return err;
    }
    err = globalValue.initialize(globalPlace.begin(), globalPlace.end(), 1);
    if (err != pando::Status::Success) {
      vec.deinitialize();
      globalPlace.deinitialize();
      localCounters.deinitialize();
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
    globalValue.deinitialize();
  }

  /**
   * @brief resets all local counters to 0 and sets globalValueComputed to false
   */
  void reset() {
    galois::doAll(
        localCounters, +[](pando::GlobalRef<T> localCounter) {
          localCounter = 0;
        });
    globalValueComputed = false;
  }

  /**
   * @brief reduce adds all local counters together, sets globalValueComputed, and returns the value
   *
   * @warning every time this is called the global value is recomputed, for subsequent access to the
   * globally computed value after the first reduce, use get instead
   */
  T reduce() {
    galois::doAll(
        this->globalValue, localCounters,
        +[](galois::DistArray<T> accumulator, pando::GlobalRef<T> localCounter) {
          T localValue = localCounter;
          galois::doAll(
              localValue, accumulator, +[](T localValue, pando::GlobalRef<T> global) {
                pando::atomicFetchAdd(&global, localValue, std::memory_order_release);
              });
        });
    globalValueComputed = true;
    return globalValue[0];
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
    return globalValue[0];
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
