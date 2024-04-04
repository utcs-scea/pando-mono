// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_HOST_CACHED_ARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_HOST_CACHED_ARRAY_HPP_

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/host_indexed_map.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace galois {

template <typename T>
class HostCachedArrayIterator;

/**
 * @brief This is an array like container that has an array on each host */
template <typename T>
class HostCachedArray {
public:
  HostCachedArray() noexcept = default;

  HostCachedArray(const HostCachedArray&) = default;
  HostCachedArray(HostCachedArray&&) = default;

  ~HostCachedArray() = default;

  HostCachedArray& operator=(const HostCachedArray&) = default;
  HostCachedArray& operator=(HostCachedArray&&) = default;

  using iterator = HostCachedArrayIterator<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  /**
   * @brief Takes in iterators with semantics like memoryType and a size to initialize the sizes of
   * the objects
   *
   * @tparam It the iterator type
   * @param[in] beg The beginning of the iterator to memoryType like objects
   * @param[in] end The end of the iterator to memoryType like objects
   * @param[in] size The size of the data to encapsulate in this abstraction
   */
  template <typename Range>
  [[nodiscard]] pando::Status initialize(Range range) {
    assert(range.size() == m_data.size());
    size_ = 0;
    PANDO_CHECK_RETURN(m_data.initialize());
    PANDO_CHECK_RETURN(galois::doAll(
        range, m_data,
        +[](Range range, pando::GlobalRef<galois::HostIndexedMap<pando::Array<T>>> data) {
          PANDO_CHECK(lift(data, initialize));
          auto ref = lift(data, getLocalRef);
          PANDO_CHECK(fmap(
              ref, initialize,
              *(range.begin() + static_cast<std::uint64_t>(pando::getCurrentPlace().node.id))));
        }));
    PANDO_CHECK_RETURN(galois::doAll(
        m_data, m_data,
        +[](decltype(m_data) complete, galois::HostIndexedMap<pando::Array<T>> data) {
          for (std::uint64_t i = 0; i < data.size(); i++) {
            data[i] = fmap(complete[i], operator[], i);
          }
        }));
    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      auto ref = fmap(m_data[i], operator[], i);
      size_ += lift(ref, size);
    }
    return pando::Status::Success;
  }

  void deinitialize() {
    PANDO_CHECK(galois::doAll(
        m_data, +[](galois::HostIndexedMap<pando::Array<T>> data) {
          const std::uint64_t i = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
          auto ref = data[i];
          liftVoid(ref, deinitialize);
          liftVoid(data, deinitialize);
        }));
    m_data.deinitialize();
  }

  /**
   * @brief Returns a pointer to the given index within a specific host
   * @warning this is unsafe
   */
  pando::GlobalPtr<T> getSpecific(std::uint64_t host, std::uint64_t localIdx) noexcept {
    HostIndexedMap<pando::Array<T>> cache = m_data.getLocalRef();
    return &fmap(cache[host], get, localIdx);
  }

  /**
   * @brief Returns a pointer to the given index within a specific host
   * @warning this is unsafe
   */
  pando::GlobalRef<T> getSpecificRef(std::uint64_t host, std::uint64_t localIdx) noexcept {
    return *this->getSpecific(host, localIdx);
  }

  /**
   * @brief Returns a pointer to the given index
   */
  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    HostIndexedMap<pando::Array<T>> cache = m_data.getLocalRef();
    auto curr = cache.begin();
    for (; curr != cache.end(); curr++) {
      auto size = lift(*curr, size);
      if (i < size) {
        break;
      }
      i -= size;
    }
    if (curr == cache.end())
      return nullptr;
    return &fmap(*curr, get, i);
  }

  /**
   * @brief Returns a pointer to the given index
   */
  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    HostIndexedMap<pando::Array<T>> cache = m_data.getLocalRef();
    auto curr = cache.begin();
    for (; curr != cache.end(); curr++) {
      auto size = lift(*curr, size);
      if (i < size) {
        break;
      }
      i -= size;
    }
    if (curr == cache.end())
      return nullptr;
    pando::GlobalRef<pando::Array<T>> arr = *curr;
    return &fmap(*curr, get, i);
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return *this->get(pos);
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return *this->get(pos);
  }

  constexpr bool empty() const noexcept {
    return this->size() == 0;
  }

  constexpr std::uint64_t size() noexcept {
    return size_;
  }

  constexpr std::uint64_t size() const noexcept {
    return size_;
  }

  constexpr std::uint64_t capacity() noexcept {
    return size();
  }

  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  iterator end() noexcept {
    return iterator(*this, size_);
  }

  iterator end() const noexcept {
    return iterator(*this, size_);
  }

  /**
   * @brief reverse iterator to the first element
   */
  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end()--);
  }

  /**
   * @copydoc rbegin()
   */
  reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end()--);
  }

  /**
   * @brief reverse iterator to the last element
   */
  reverse_iterator rend() noexcept {
    return reverse_iterator(begin()--);
  }

  /**
   * @copydoc rend()
   */
  reverse_iterator rend() const noexcept {
    return reverse_iterator(begin()--);
  }

  friend bool operator==(const HostCachedArray& a, const HostCachedArray& b) {
    return a.size() == b.size() && a.m_data.getLocal() == b.m_data.getLocal();
  }

private:
  /// @brief The data structure storing the data this stores a cache once constructed
  galois::HostLocalStorage<galois::HostIndexedMap<pando::Array<T>>> m_data;
  /// @brief Stores the amount of data in the array, may be less than allocated
  uint64_t size_ = 0;
};

/**
 * @brief an iterator that stores the DistArray and the current position to provide random access
 * iterator semantics
 */
template <typename T>
class HostCachedArrayIterator {
  HostCachedArray<T> m_arr;
  std::uint64_t m_pos;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  HostCachedArrayIterator(HostCachedArray<T> arr, std::uint64_t pos) : m_arr(arr), m_pos(pos) {}

  constexpr HostCachedArrayIterator() noexcept = default;
  constexpr HostCachedArrayIterator(HostCachedArrayIterator&&) noexcept = default;
  constexpr HostCachedArrayIterator(const HostCachedArrayIterator&) noexcept = default;
  ~HostCachedArrayIterator() = default;

  constexpr HostCachedArrayIterator& operator=(const HostCachedArrayIterator&) noexcept = default;
  constexpr HostCachedArrayIterator& operator=(HostCachedArrayIterator&&) noexcept = default;

  reference operator*() const noexcept {
    return m_arr[m_pos];
  }

  reference operator*() noexcept {
    return m_arr[m_pos];
  }

  pointer operator->() {
    return m_arr.get(m_pos);
  }

  HostCachedArrayIterator& operator++() {
    m_pos++;
    return *this;
  }

  HostCachedArrayIterator operator++(int) {
    HostCachedArrayIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  HostCachedArrayIterator& operator--() {
    m_pos--;
    return *this;
  }

  HostCachedArrayIterator operator--(int) {
    HostCachedArrayIterator tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr HostCachedArrayIterator operator+(std::uint64_t n) const noexcept {
    return HostCachedArrayIterator(m_arr, m_pos + n);
  }

  constexpr HostCachedArrayIterator& operator+=(std::uint64_t n) noexcept {
    m_pos += n;
    return *this;
  }

  constexpr HostCachedArrayIterator operator-(std::uint64_t n) const noexcept {
    return HostCachedArrayIterator(m_arr, m_pos - n);
  }

  constexpr difference_type operator-(HostCachedArrayIterator b) const noexcept {
    return m_pos - b.m_pos;
  }

  reference operator[](std::uint64_t n) noexcept {
    return m_arr[m_pos + n];
  }

  reference operator[](std::uint64_t n) const noexcept {
    return m_arr[m_pos + n];
  }

  friend bool operator==(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return a.m_pos == b.m_pos && a.m_arr == b.m_arr;
  }

  friend bool operator!=(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return !(a == b);
  }

  friend bool operator<(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return a.m_pos < b.m_pos;
  }

  friend bool operator<=(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return a.m_pos <= b.m_pos;
  }

  friend bool operator>(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return a.m_pos > b.m_pos;
  }

  friend bool operator>=(const HostCachedArrayIterator& a, const HostCachedArrayIterator& b) {
    return a.m_pos >= b.m_pos;
  }

  friend pando::Place localityOf(HostCachedArrayIterator& a) {
    pando::GlobalPtr<T> ptr = &a.m_arr[a.m_pos];
    return pando::localityOf(ptr);
  }
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_HOST_CACHED_ARRAY_HPP_
