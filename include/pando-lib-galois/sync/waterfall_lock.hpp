// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_WATERFALL_LOCK_HPP_
#define PANDO_LIB_GALOIS_SYNC_WATERFALL_LOCK_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory.hpp>
#include <pando-rt/sync/atomic.hpp>

namespace galois {

/** This is a Barrier style lock used for fine grained release control
 * @author AdityaAtulTewari
 */
template <typename T>
class WaterFallLock {
  T wfc;

public:
  WaterFallLock() = default;

  [[nodiscard]] pando::Status initialize(std::uint64_t size) {
    pando::Status err = wfc.initialize(size);
    if (err != pando::Status::Success) {
      return err;
    }
    reset();
    return pando::Status::Success;
  }
  void deinitialize() {
    wfc.deinitialize();
  }

  void reset() {
    for (unsigned i = 0; i < wfc.size(); i++)
      *(wfc.get(i)) = 0;
  }

  const char* name() {
    return typeid(WaterFallLock<T>).name();
  }

  template <unsigned val>
  void wait(uint64_t num) {
    while (pando::atomicLoad(wfc.get(num), std::memory_order_acquire) != val) {}
  }
  template <unsigned val>
  void done(uint64_t num) {
    pando::atomicStore(wfc.get(num), val, std::memory_order_release);
  }
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_SYNC_WATERFALL_LOCK_HPP_
