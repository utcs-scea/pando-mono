// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace galois {

/**
 * @brief a place and a memoryType for use in constructing a DistArray
 */
struct PlaceType {
  pando::Place place;
  pando::MemoryType memType;
};

template <typename T>
struct DAIterator;

/**
 * @brief This is an array like container that spans multiple hosts
 */
template <typename T>
class DistArray {
private:
  struct FromState {
    FromState() = default;
    FromState(galois::DistArray<T> to_, pando::Vector<T> from_) : to(to_), from(from_) {}

    galois::DistArray<T> to;
    pando::Vector<T> from;
  };

public:
  /// @brief The data structure storing the data
  pando::Array<pando::Array<T>> m_data;
  /// @brief Stores the amount of data in the array, may be less than allocated
  uint64_t size_ = 0;

  /**
   * @brief Returns a pointer to the given index
   */
  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    if (m_data.size() == 0 || i >= m_data.size() * static_cast<pando::Array<T>>(m_data[0]).size()) {
      return nullptr;
    }
    pando::Array<T> arr = m_data[0];
    const std::uint64_t index = i % arr.size();
    const std::uint64_t blockId = i / arr.size();
    pando::Array<T> blockPtr = m_data[blockId];

    return &(blockPtr[index]);
  }

  /**
   * @brief Returns a pointer to the given index
   */
  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    if (m_data.size() == 0 || i >= m_data.size() * static_cast<pando::Array<T>>(m_data[0]).size()) {
      return nullptr;
    }
    pando::Array<T> arr = m_data[0];
    const std::uint64_t index = i % arr.size();
    const std::uint64_t blockId = i / arr.size();
    pando::Array<T> blockPtr = m_data[blockId];

    return &(blockPtr[index]);
  }

  friend DAIterator<T>;

  using iterator = DAIterator<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using pointer = pando::GlobalPtr<T>;

  DistArray() noexcept = default;

  DistArray(const DistArray&) = default;
  DistArray(DistArray&&) = default;

  ~DistArray() = default;

  DistArray& operator=(const DistArray&) = default;
  DistArray& operator=(DistArray&&) = default;

  /**
   * @brief Takes in iterators with semantics like memoryType and a size to initialize the sizes of
   * the objects
   *
   * @tparam It the iterator type
   * @param[in] beg The beginning of the iterator to memoryType like objects
   * @param[in] end The end of the iterator to memoryType like objects
   * @param[in] size The size of the data to encapsulate in this abstraction
   */
  template <typename It>
  [[nodiscard]] pando::Status initialize(It beg, It end, std::uint64_t size) {
    if (m_data.data() != nullptr) {
      return pando::Status::AlreadyInit;
    }
    size_ = size;

    if (size == 0) {
      return pando::Status::Success;
    }

    const std::uint64_t buckets = std::distance(beg, end);

    const std::uint64_t bucketSize = (size / buckets) + (size % buckets ? 1 : 0);

    pando::Status err = m_data.initialize(buckets);
    if (err != pando::Status::Success) {
      return err;
    }

    for (std::uint64_t i = 0; beg != end; i++, beg++) {
      pando::Array<T> arr;
      typename std::iterator_traits<It>::value_type val = *beg;
      err = arr.initialize(bucketSize, val.place, val.memType);
      if (err != pando::Status::Success) {
        return err;
      }
      m_data[i] = arr;
    }
    return err;
  }

  /**
   * @brief Assumes the DistArray should have elements on all hosts and initializes the sizes of
   * the objects
   *
   * @param[in] size The size of the data to encapsulate in this abstraction
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size) {
    size_ = size;
    pando::Array<PlaceType> hostsPlaces;
    pando::Status err = hostsPlaces.initialize(pando::getPlaceDims().node.id);
    if (err != pando::Status::Success) {
      return err;
    }
    for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
      hostsPlaces[i] = PlaceType{pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                                 pando::MemoryType::Main};
    }
    err = initialize(hostsPlaces.begin(), hostsPlaces.end(), size);
    hostsPlaces.deinitialize();
    return err;
  }

  [[nodiscard]] pando::Status from(pando::Vector<T> data, uint64_t size) {
    pando::Status err = initialize(size);
    if (err != pando::Status::Success) {
      return err;
    }
    galois::onEach(
        FromState(*this, data), +[](FromState& state, uint64_t thread, uint64_t total_threads) {
          uint64_t workPerHost = state.to.size() / total_threads;
          uint64_t start = thread * workPerHost;
          uint64_t end = start + workPerHost;
          if (thread == total_threads - 1) {
            end = state.to.size();
          }
          for (uint64_t i = 0; i < end; i++) {
            state.to[i] = state.from[i];
          }
        });
    return err;
  }

  /**
   * @brief Deinitializes the array.
   */
  void deinitialize() {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Array only supports trivially destructible types");

    if (m_data.data() == nullptr) {
      return;
    }
    for (pando::Array<T> array : m_data) {
      array.deinitialize();
    }
    m_data.deinitialize();
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return *get(pos);
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return *get(pos);
  }

  constexpr bool empty() const noexcept {
    return m_data.size() == 0;
  }

  constexpr std::uint64_t size() noexcept {
    return size_;
  }

  constexpr std::uint64_t size() const noexcept {
    return size_;
  }

  /**
   * @brief A beginning local iterator for a specified node `n` that
   * points to the first local item of this distributed array.
   */
  pando::GlobalPtr<T> localBegin(std::int16_t n) noexcept {
    pando::Array<T> arr = m_data[n];
    return pando::GlobalPtr<T>(arr.begin());
  }

  /**
   * @brief A ending local iterator for a specified node `n` that
   * points to (the last local item of this distributed array + 1).
   */
  pando::GlobalPtr<T> localEnd(std::int16_t n) noexcept {
    pando::Array<T> arr = m_data[n];
    return pando::GlobalPtr<T>(arr.end());
  }

  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  iterator end() noexcept {
    return iterator(*this, size());
  }

  iterator end() const noexcept {
    return iterator(*this, size());
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

  friend bool isSame(const DistArray<T>& a, const DistArray<T>& b) {
    return a.m_data.data() == b.m_data.data();
  }
};

/**
 * @brief an iterator that stores the DistArray and the current position to provide random access
 * iterator semantics
 */
template <typename T>
class DAIterator {
  DistArray<T> m_arr;
  std::uint64_t m_pos;
  friend CountedIterator<DAIterator<T>>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  DAIterator(DistArray<T> arr, std::uint64_t pos) : m_arr(arr), m_pos(pos) {}

  constexpr DAIterator() noexcept = default;
  constexpr DAIterator(DAIterator&&) noexcept = default;
  constexpr DAIterator(const DAIterator&) noexcept = default;
  ~DAIterator() = default;

  constexpr DAIterator& operator=(const DAIterator&) noexcept = default;
  constexpr DAIterator& operator=(DAIterator&&) noexcept = default;

  reference operator*() const noexcept {
    return *m_arr.get(m_pos);
  }

  reference operator*() noexcept {
    return *m_arr.get(m_pos);
  }

  pointer operator->() {
    return m_arr.get(m_pos);
  }

  DAIterator& operator++() {
    m_pos++;
    return *this;
  }

  DAIterator operator++(int) {
    DAIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  DAIterator& operator--() {
    m_pos--;
    return *this;
  }

  DAIterator operator--(int) {
    DAIterator tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr DAIterator operator+(std::uint64_t n) const noexcept {
    return DAIterator(m_arr, m_pos + n);
  }

  constexpr DAIterator& operator+=(std::uint64_t n) noexcept {
    m_pos += n;
    return *this;
  }

  constexpr DAIterator operator-(std::uint64_t n) const noexcept {
    return DAIterator(m_arr, m_pos - n);
  }

  constexpr difference_type operator-(DAIterator b) const noexcept {
    return m_pos - b.m_pos;
  }

  reference operator[](std::uint64_t n) noexcept {
    return m_arr[m_pos + n];
  }

  reference operator[](std::uint64_t n) const noexcept {
    return m_arr[m_pos + n];
  }

  friend bool operator==(const DAIterator& a, const DAIterator& b) {
    return a.m_pos == b.m_pos && a.m_arr.size() == b.m_arr.size() &&
           a.m_arr.m_data.data() == b.m_arr.m_data.data();
  }

  friend bool operator!=(const DAIterator& a, const DAIterator& b) {
    return !(a == b);
  }

  friend bool operator<(const DAIterator& a, const DAIterator& b) {
    return a.m_pos < b.m_pos;
  }

  friend bool operator<=(const DAIterator& a, const DAIterator& b) {
    return a.m_pos <= b.m_pos;
  }

  friend bool operator>(const DAIterator& a, const DAIterator& b) {
    return a.m_pos > b.m_pos;
  }

  friend bool operator>=(const DAIterator& a, const DAIterator& b) {
    return a.m_pos >= b.m_pos;
  }

  friend pando::Place localityOf(DAIterator& a) {
    pando::GlobalPtr<T> ptr = &a.m_arr[a.m_pos];
    return pando::localityOf(ptr);
  }
};

/**
 * @brief a Slice of a DistArray, so there is a start and end of the slice
 */
template <typename T>
class DistArraySlice {
  DistArray<T> m_arr;
  std::uint64_t m_begin;
  std::uint64_t m_end;

public:
  using iterator = CountedIterator<DAIterator<T>>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  DistArraySlice(DistArray<T> arr, std::uint64_t begin, std::uint64_t end)
      : m_arr(arr), m_begin(begin), m_end(end) {}

  DistArraySlice() noexcept = default;
  DistArraySlice(const DistArraySlice&) = default;
  DistArraySlice(DistArraySlice&&) = default;

  ~DistArraySlice() = default;

  DistArraySlice& operator=(const DistArraySlice&) = default;
  DistArraySlice& operator=(DistArraySlice&&) = default;

  constexpr std::uint64_t size() noexcept {
    return m_end - m_begin;
  }

  constexpr std::uint64_t size() const noexcept {
    return m_end - m_begin;
  }

  iterator begin() noexcept {
    return iterator(m_begin, DAIterator<T>(m_arr, m_begin));
  }

  iterator begin() const noexcept {
    return iterator(m_begin, DAIterator<T>(m_arr, m_begin));
  }

  iterator end() noexcept {
    return iterator(m_end, DAIterator<T>(m_arr, m_end));
  }

  iterator end() const noexcept {
    return iterator(m_end, DAIterator<T>(m_arr, m_end));
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
};

} // namespace galois

namespace std {
template <typename T>
struct iterator_traits<galois::DAIterator<T>> {
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;
  using difference_type = std::int64_t;
  using iterator_category = std::random_access_iterator_tag;
};
} // namespace std

#endif // PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_
