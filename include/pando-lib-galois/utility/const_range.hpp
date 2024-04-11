// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_CONST_RANGE_HPP_
#define PANDO_LIB_GALOIS_UTILITY_CONST_RANGE_HPP_

#include <pando-rt/export.h>

#include <cstdint>

#include <pando-rt/pando-rt.hpp>

namespace galois {

template <typename T>
class ConstRange {
  std::uint64_t m_count;
  const T m_val;

public:
  using iterator = ConstRange<T>;
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = const T;

  ConstRange(std::uint64_t pos, T val) : m_count(pos), m_val(val) {}

  ConstRange() noexcept = default;
  ConstRange(const ConstRange&) = default;
  ConstRange(ConstRange&&) = default;

  ~ConstRange() = default;

  ConstRange& operator=(const ConstRange&) = default;
  ConstRange& operator=(ConstRange&&) = default;

  ConstRange<T> begin() const noexcept {
    return ConstRange(0, m_val);
  }

  ConstRange<T> end() const noexcept {
    return ConstRange<T>(m_count, m_val);
  }

  constexpr std::uint64_t size() const noexcept {
    return m_count;
  }

  value_type operator*() const noexcept {
    return m_val;
  }

  ConstRange<T>& operator++() {
    m_count++;
    return *this;
  }

  ConstRange<T> operator++(int) {
    ConstRange<T> tmp = *this;
    ++(*this);
    return tmp;
  }

  ConstRange<T>& operator--() {
    m_count--;
    return *this;
  }

  ConstRange<T> operator--(int) {
    ConstRange<T> tmp = *this;
    --(*this);
    return tmp;
  }

  ConstRange<T> operator+(std::uint64_t n) {
    return ConstRange<T>{m_count + n, m_val};
  }

  friend bool operator==(const ConstRange<T>& a, const ConstRange<T>& b) {
    return a.m_count == b.m_count && a.m_val == b.m_val;
  }

  friend bool operator!=(const ConstRange<T>& a, const ConstRange<T>& b) {
    return !(a == b);
  }

  friend bool operator<(const ConstRange<T>& a, const ConstRange<T>& b) {
    return a.m_count < b.m_count;
  }

  friend bool operator<=(const ConstRange<T>& a, const ConstRange<T>& b) {
    return a.m_count <= b.m_count;
  }

  friend bool operator>(const ConstRange<T>& a, const ConstRange<T>& b) {
    return a.m_count > b.m_count;
  }

  friend bool operator>=(const ConstRange<T>& a, const ConstRange<T>& b) {
    return a.m_count >= b.m_count;
  }

  friend pando::Place localityOf(ConstRange<T>& a) {
    return pando::getCurrentPlace();
  }
};

} // namespace galois
#endif // PANDO_LIB_GALOIS_UTILITY_CONST_RANGE_HPP_
