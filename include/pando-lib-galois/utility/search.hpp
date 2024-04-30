// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_SEARCH_HPP_
#define PANDO_LIB_GALOIS_UTILITY_SEARCH_HPP_

namespace galois {

template <typename RandomAccessIterator, typename T, typename F>
RandomAccessIterator lower_bound(RandomAccessIterator start, RandomAccessIterator end, const T& val,
                                 F func) {
  RandomAccessIterator lb = start, ub = end;
  while (lb < ub) {
    RandomAccessIterator mid = lb + (ub - lb) / 2;
    if (func(mid, val)) {
      lb = mid + 1;
    } else {
      ub = mid;
    }
  }
  return ub;
}

template <typename RandomAccessIterator, typename T>
RandomAccessIterator lower_bound(RandomAccessIterator start, RandomAccessIterator end,
                                 const T& val) {
  return lower_bound(
      start, end, +[](RandomAccessIterator mid, const T& val) {
        return *mid < val;
      });
}

template <typename RandomAccessIterator, typename T>
bool binary_search(RandomAccessIterator start, RandomAccessIterator end, const T& val) {
  RandomAccessIterator ub = lower_bound(start, end, val);
  return (ub < end) && (*ub == val);
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_UTILITY_SEARCH_HPP_
