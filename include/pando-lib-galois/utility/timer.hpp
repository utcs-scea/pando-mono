// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_TIMER_HPP_
#define PANDO_LIB_GALOIS_UTILITY_TIMER_HPP_

#include <chrono>
#include <cstdio>

namespace galois {

using Clock = std::chrono::high_resolution_clock;

struct Timer {
  explicit Timer(const char* end_msg_, bool microseconds_ = false)
      : start_msg(nullptr), end_msg(end_msg_), finished(false), microseconds(microseconds_) {
#ifndef DISABLE_TIMERS
    start = Clock::now();
#endif
  }
  Timer(const char* start_msg_, const char* end_msg_, bool microseconds_ = false)
      : start_msg(start_msg_), end_msg(end_msg_), finished(false), microseconds(microseconds_) {
#ifndef DISABLE_TIMERS
    start = Clock::now();
    std::printf("%s\n", start_msg);
#endif
  }
  ~Timer() {
    Stop();
  }

  uint64_t GetDuration(const std::chrono::time_point<Clock>& first,
                       const std::chrono::time_point<Clock>& second) {
    if (microseconds) {
      return std::chrono::duration_cast<std::chrono::microseconds>(second - first).count();
    } else {
      return std::chrono::duration_cast<std::chrono::milliseconds>(second - first).count();
    }
  }
  uint64_t GetDuration() {
    return GetDuration(start, end);
  }

  const char* TimeUnit() {
    if (microseconds) {
      return "us";
    } else {
      return "ms";
    }
  }

  void Stop() {
    if (finished) {
      return;
    }
    finished = true;
#ifndef DISABLE_TIMERS
    end = Clock::now();
    std::printf("%s, Elapsed Time: %lu%s\n", end_msg, GetDuration(), TimeUnit());
#endif
  }

  std::chrono::time_point<Clock> start;
  std::chrono::time_point<Clock> end;
  const char* start_msg;
  const char* end_msg;
  bool finished;
  bool microseconds;
} typedef Timer;

} // namespace galois

#endif // PANDO_LIB_GALOIS_UTILITY_TIMER_HPP_
