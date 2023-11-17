// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/sync/atomic.hpp>

#include <cstdint>

#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>

namespace {

inline void convertDoubleToU64(double x, uint64_t& res) {
  memcpy(&res, &x, sizeof(uint64_t));
}

} // namespace

double pando::atomicFetchAdd(pando::GlobalPtr<double> ptr, double value) {
  pando::GlobalPtr<uint64_t> u64_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::uint64_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  uint64_t expected = *u64_ptr;
  uint64_t desired;
  convertDoubleToU64(*ptr + value, desired);
  while (pando::atomicCompareExchange(u64_ptr, expected, desired) != expected) {
    expected = *u64_ptr;
    convertDoubleToU64(*ptr + value, desired);
  }
  return *ptr;
}
double pando::atomicFetchSub(pando::GlobalPtr<double> ptr, double value) {
  pando::GlobalPtr<uint64_t> u64_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::uint64_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  uint64_t expected = *u64_ptr;
  uint64_t desired;
  convertDoubleToU64(*ptr - value, desired);
  while (pando::atomicCompareExchange(u64_ptr, expected, desired) != expected) {
    expected = *u64_ptr;
    convertDoubleToU64(*ptr - value, desired);
  }
  return *ptr;
}
