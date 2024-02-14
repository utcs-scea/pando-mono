// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/sync/atomic.hpp>

#include <cstdint>

#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>

namespace {

inline void convertDoubleToU64(double x, uint64_t& res) {
  char* from = reinterpret_cast<char*>(&x);
  char* to = reinterpret_cast<char*>(&res);
  for (std::uint64_t i = 0; i < sizeof(uint64_t); i++) {
    to[i] = from[i];
  }
}

inline void convertFloatToI32(float x, int32_t& res) {
  char* from = reinterpret_cast<char*>(&x);
  char* to = reinterpret_cast<char*>(&res);
  for (std::uint64_t i = 0; i < sizeof(int32_t); i++) {
    to[i] = from[i];
  }
}

} // namespace

double pando::atomicFetchAdd(pando::GlobalPtr<double> ptr, double value) {
  return pando::atomicFetchAdd(ptr, value, std::memory_order_seq_cst, std::memory_order_relaxed);
}

double pando::atomicFetchSub(pando::GlobalPtr<double> ptr, double value) {
  return atomicFetchSub(ptr, value, std::memory_order_seq_cst, std::memory_order_relaxed);
}

double pando::atomicLoad(pando::GlobalPtr<double> ptr, std::memory_order order) {
  pando::atomicThreadFence(order);
  return *ptr;
}

double pando::atomicFetchAdd(pando::GlobalPtr<double> ptr, double value,
                             std::memory_order memoryOrder) {
  return pando::atomicFetchAdd(ptr, value, memoryOrder, std::memory_order_relaxed);
}

double pando::atomicFetchAdd(pando::GlobalPtr<double> ptr, double value, std::memory_order,
                             std::memory_order) {
  pando::GlobalPtr<uint64_t> u64_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::uint64_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  uint64_t desired;
  uint64_t expected;
  double original;
  do {
    original = *ptr;
    convertDoubleToU64(original, expected);
    convertDoubleToU64(original + value, desired);
  } while (pando::atomicCompareExchange(u64_ptr, expected, desired) != expected);
  return original;
}

double pando::atomicFetchSub(pando::GlobalPtr<double> ptr, double value, std::memory_order,
                             std::memory_order) {
  pando::GlobalPtr<uint64_t> u64_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::uint64_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  uint64_t desired;
  uint64_t expected;
  double original;
  do {
    original = *ptr;
    convertDoubleToU64(original, expected);
    convertDoubleToU64(original - value, desired);
  } while (pando::atomicCompareExchange(u64_ptr, expected, desired) != expected);
  return original;
}

double pando::atomicLoad(pando::GlobalPtr<double> ptr) {
  return atomicLoad(ptr, std::memory_order_seq_cst);
}

float pando::atomicLoad(pando::GlobalPtr<float> ptr) {
  return pando::atomicLoad(ptr, std::memory_order_seq_cst);
}

float pando::atomicLoad(pando::GlobalPtr<float> ptr, std::memory_order memoryOrder) {
  pando::atomicThreadFence(memoryOrder);
  return *ptr;
}

float pando::atomicFetchAdd(pando::GlobalPtr<float> ptr, float value) {
  return atomicFetchAdd(ptr, value, std::memory_order_seq_cst, std::memory_order_relaxed);
}

float pando::atomicFetchSub(pando::GlobalPtr<float> ptr, float value) {
  return atomicFetchSub(ptr, value, std::memory_order_seq_cst, std::memory_order_relaxed);
}

float pando::atomicFetchAdd(pando::GlobalPtr<float> ptr, float value, std::memory_order,
                            std::memory_order) {
  pando::GlobalPtr<int32_t> i32_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::int32_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  int32_t desired;
  int32_t expected;
  float original;
  do {
    original = *ptr;
    convertFloatToI32(original, expected);
    convertFloatToI32(original + value, desired);
  } while (pando::atomicCompareExchange(i32_ptr, expected, desired) != expected);
  return original;
}

float pando::atomicFetchSub(pando::GlobalPtr<float> ptr, float value, std::memory_order,
                            std::memory_order) {
  pando::GlobalPtr<int32_t> i32_ptr =
      pando::globalPtrReinterpretCast<pando::GlobalPtr<std::int32_t>>(ptr);
  // this is not a race since we set the expected value first and then our desired value
  // if the internal value of the ptr changes between setting these values then the CAS
  // will fail
  int32_t desired;
  int32_t expected;
  float original;
  do {
    original = *ptr;
    convertFloatToI32(original, expected);
    convertFloatToI32(original - value, desired);
  } while (pando::atomicCompareExchange(i32_ptr, expected, desired) != expected);
  return original;
}
