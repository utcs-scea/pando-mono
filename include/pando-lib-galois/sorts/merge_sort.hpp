// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SORTS_MERGE_SORT_HPP_
#define PANDO_LIB_GALOIS_SORTS_MERGE_SORT_HPP_

#include <pando-rt/export.h>

#include <algorithm>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/wait_group.hpp>

namespace galois {

/**
 * @brief a function to merge 2 vectors
 */
template <class T>
void merge(pando::Vector<T> arr, pando::Vector<T> temp, std::int64_t l1, std::int64_t r1,
           std::int64_t l2, std::int64_t r2, bool (*comp)(T, T)) {
  std::uint64_t index = l1;
  while (l1 <= r1 && l2 <= r2) {
    if (!comp(arr[l1], arr[l2])) {
      pando::GlobalRef<T> t_ref = temp[index];
      t_ref = arr[l1];
      l1++;
    } else {
      pando::GlobalRef<T> t_ref = temp[index];
      t_ref = arr[l2];
      l2++;
    }
    index++;
  }
  while (l1 <= r1) {
    pando::GlobalRef<T> t_ref = temp[index];
    t_ref = arr[l1];
    index++;
    l1++;
  }
  while (l2 <= r2) {
    pando::GlobalRef<T> t_ref = temp[index];
    t_ref = arr[l2];
    index++;
    l2++;
  }
}

/**
 * @brief a function to iteratively merge sort a vector upto the first n values.
 */
template <class T>
void merge_sort_n(pando::Vector<T> arr, bool (*comp)(T, T), std::uint64_t n) {
  std::uint64_t len = 1;

  pando::Vector<T> temp;
  PANDO_CHECK(temp.initialize(n));

  while (len < n) {
    std::uint64_t i = 0;
    while (i < n) {
      std::uint64_t l1 = i;
      std::uint64_t r1 = i + len - 1;
      std::uint64_t l2 = i + len;
      std::uint64_t r2 = std::min(n - 1, i + 2 * len - 1);

      if (l2 > n)
        break;

      merge<T>(arr, temp, l1, r1, l2, r2, comp);
      for (std::uint64_t j = l1; j <= r2; j++) {
        pando::GlobalRef<T> a_ref = arr[j];
        a_ref = temp[j];
      }
      i = i + 2 * len;
    }
    len = len * 2;
  }
  temp.deinitialize();
}

/**
 * @brief a function to iteratively merge sort a vector
 */
template <class T>
void merge_sort(pando::Vector<T> arr, bool (*comp)(T, T)) {
  merge_sort_n(arr, comp, arr.size());
}

} // namespace galois
#endif // PANDO_LIB_GALOIS_SORTS_MERGE_SORT_HPP_
