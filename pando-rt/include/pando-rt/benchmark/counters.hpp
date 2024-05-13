// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_RT_BENCHMARK_COUNTERS_HPP_
#define PANDO_RT_BENCHMARK_COUNTERS_HPP_

#include <array>
#include <cstdint>
#include <chrono>

#include <pando-rt/locality.hpp>


template<typename T>
struct Record {
  std::array<T,66> counts;

  constexpr Record() {
    for(auto& count : counts) {
      count = T();
    }
  }

  Record(Record&) = delete;
  Record& operator=(Record&) = delete;
  Record(Record&&) = default;
  Record& operator=(Record&&) = default;

  void reset () {
    for(auto& count : counts) {
      count = T();
    }
  }

  template <typename A, typename F>
  void record(A val, F func, bool isOnCP,
      decltype(pando::getCurrentPlace().core.x) corex,
      decltype(pando::getCurrentPlace().core.x) coreDims) {
    std::uint64_t idx = isOnCP ? coreDims + 1 : corex;
    counts[idx] += func(val);
  }

  template <typename A, typename F>
  void record(A val, F func) {
    auto thisPlace = pando::getCurrentPlace();
    auto coreDims = pando::getCoreDims();
    record(val, func, pando::isOnCP(), thisPlace.core.x, coreDims.x);
  }

  T& get(std::uint64_t i) {
    return counts[i];
  }
};

struct HighResolutionCount {
  std::chrono::time_point<std::chrono::high_resolution_clock> begin;

  inline void start() {
    begin = std::chrono::high_resolution_clock::now();
  }

  inline std::chrono::nanoseconds stop() const noexcept{
    auto stop = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(stop - begin);
  }

  inline static void recordHighResolutionEvent(Record<std::int64_t>& r,
      HighResolutionCount c, bool isOnCP,
      decltype(pando::getCurrentPlace().core.x) corex,
      decltype(pando::getCurrentPlace().core.x) coreDims) {
    r.record(c, [](const HighResolutionCount& c) {
        return c.stop().count();
    }, isOnCP, corex, coreDims);
  }

  inline static void recordHighResolutionEvent(Record<std::int64_t>& r, HighResolutionCount c) {
    r.record(c, [](const HighResolutionCount& c){
        return c.stop().count();
    });
  }
};
#endif
