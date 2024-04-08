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
struct CountedIterator {
  std::uint64_t m_count;
  It m_iter;

  template <typename T>
  struct RetType {
    std::uint64_t curr;
    T value;
    static_assert(!std::is_same<std::uint64_t, T>::value);
    operator T() {
      return value;
    }
    operator std::uint64_t() {
      return curr;
    }
  };
  using iterator_category = typename std::iterator_traits<It>::iterator_category;
  using difference_type = std::int64_t;
  using value_type = RetType<typename std::iterator_traits<It>::value_type>;
  using pointer = RetType<typename std::iterator_traits<It>::pointer>;
  using reference = RetType<typename std::iterator_traits<It>::reference>;

  CountedIterator(std::uint64_t pos, It iter) : m_count(pos), m_iter(iter) {}

  CountedIterator() noexcept = default;
  CountedIterator(const CountedIterator&) = default;
  CountedIterator(CountedIterator&&) = default;

  ~CountedIterator() = default;

  CountedIterator& operator=(const CountedIterator&) = default;
  CountedIterator& operator=(CountedIterator&&) = default;

  reference operator*() const noexcept {
    return reference{m_count, *m_iter};
  }

  reference operator*() noexcept {
    return reference{m_count, *m_iter};
  }

  pointer operator->() {
    return pointer{m_count, &(*m_iter)};
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

  CountedIterator<It> operator+(std::uint64_t n) {
    return CountedIterator<It>{m_count + n, m_iter + n};
  }

  friend bool operator==(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count == b.m_count && a.m_iter == b.m_iter;
  }

  friend bool operator!=(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return !(a == b);
  }

  friend bool operator<(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count < b.m_count && a.m_iter < b.m_iter;
  }

  friend bool operator<=(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count <= b.m_count && a.m_iter <= b.m_iter;
  }

  friend bool operator>(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count > b.m_count && a.m_iter > b.m_iter;
  }

  friend bool operator>=(const CountedIterator<It>& a, const CountedIterator<It>& b) {
    return a.m_count >= b.m_count && a.m_iter >= b.m_iter;
  }

  friend pando::Place localityOf(CountedIterator<It>& a) {
    return localityOf(a.m_iter);
  }
};

/**
 * @brief Specialization of CountedIterator pointing to nothing
 */
template <>
class CountedIterator<void> {
  std::uint64_t m_count;

public:
  using difference_type = std::int64_t;
  using value_type = std::uint64_t;

  constexpr explicit CountedIterator(std::uint64_t pos) : m_count(pos) {}

  CountedIterator() noexcept = default;
  CountedIterator(const CountedIterator&) = default;
  CountedIterator(CountedIterator&&) = default;

  ~CountedIterator() = default;

  CountedIterator& operator=(const CountedIterator&) = default;
  CountedIterator& operator=(CountedIterator&&) = default;

  value_type operator*() const noexcept {
    return m_count;
  }

  value_type operator*() noexcept {
    return m_count;
  }

  CountedIterator<void>& operator++() {
    m_count++;
    return *this;
  }

  CountedIterator<void> operator++(int) {
    CountedIterator<void> tmp = *this;
    ++(*this);
    return tmp;
  }

  CountedIterator<void>& operator--() {
    m_count--;
    return *this;
  }

  CountedIterator<void> operator--(int) {
    CountedIterator<void> tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const CountedIterator<void>& a, const CountedIterator<void>& b) {
    return a.m_count == b.m_count;
  }

  friend bool operator!=(const CountedIterator<void>& a, const CountedIterator<void>& b) {
    return !(a == b);
  }

  friend pando::Place localityOf(CountedIterator<void>& a) {
    (void)a;
    return pando::getCurrentPlace();
  }
};

/**
 * @brief Simple range that represents the beginning and end of a range that isn't specific to any
 * structure.
 */
class IotaRange {
  std::uint64_t m_beg;
  std::uint64_t m_end;

public:
  IotaRange(std::uint64_t begin, std::uint64_t end) : m_beg(begin), m_end(end) {}
  IotaRange() noexcept = default;

  IotaRange(const IotaRange&) = default;
  IotaRange(IotaRange&&) = default;

  ~IotaRange() = default;

  IotaRange& operator=(const IotaRange&) = default;
  IotaRange& operator=(IotaRange&&) = default;

  using iterator = CountedIterator<void>;

  constexpr iterator begin() noexcept {
    return iterator(m_beg);
  }

  constexpr iterator end() noexcept {
    return iterator(m_end);
  }

  constexpr std::int64_t size() {
    return m_end - m_beg;
  }
};

} // namespace galois
#endif // PANDO_LIB_GALOIS_UTILITY_COUNTED_ITERATOR_HPP_
