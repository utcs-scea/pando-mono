// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_COUNTED_ITERATOR_HPP_
#define PANDO_LIB_GALOIS_UTILITY_COUNTED_ITERATOR_HPP_

#include <pando-rt/export.h>

#include <cstdint>
#include <utility>

#include <pando-rt/pando-rt.hpp>

namespace galois {

template <typename It>
class CountedIterator {
  std::uint64_t m_count;
  It m_iter;

public:
  using iterator_category = typename std::iterator_traits<It>::iterator_category;
  using difference_type = std::int64_t;
  using value_type = std::pair<std::uint64_t, typename std::iterator_traits<It>::value_type>;
  using pointer = std::pair<std::uint64_t, typename std::iterator_traits<It>::pointer>;
  using reference = std::pair<std::uint64_t, typename std::iterator_traits<It>::reference>;

  CountedIterator(std::uint64_t pos, It iter) : m_count(pos), m_iter(iter) {}

  CountedIterator() noexcept = default;
  CountedIterator(const CountedIterator&) = default;
  CountedIterator(CountedIterator&&) = default;

  ~CountedIterator() = default;

  CountedIterator& operator=(const CountedIterator&) = default;
  CountedIterator& operator=(CountedIterator&&) = default;

  reference operator*() const noexcept {
    return std::make_pair(m_count, *m_iter);
  }

  reference operator*() noexcept {
    return std::make_pair(m_count, *m_iter);
  }

  pointer operator->() {
    return std::make_pair(m_count, &(*m_iter));
  }

  CountedIterator<It>& operator++() {
    m_count++;
    m_iter++;
    return *this;
  }

  CountedIterator<It> operator++(int) {
    CountedIterator<It> tmp = *this;
    ++(*this);
    return tmp;
  }

  CountedIterator<It>& operator--() {
    m_count--;
    m_iter--;
    return *this;
  }

  CountedIterator<It> operator--(int) {
    CountedIterator<It> tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count == b.m_count && a.m_iter == b.m_iter;
  }

  friend bool operator!=(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return !(a == b);
  }

  friend pando::Place localityOf(CountedIterator<It>& a) {
    return localityOf(a.m_iter);
  }
};

} // namespace galois
#endif // PANDO_LIB_GALOIS_UTILITY_COUNTED_ITERATOR_HPP_
