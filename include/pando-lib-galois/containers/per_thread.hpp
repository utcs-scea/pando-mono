// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace galois {

template <typename T>
struct PTVectorIterator;

/**
 * @brief This is a basic mechanism for appending data to thread local vectors.
 *
 * We expect any work pushed to the vectors to be handled by another host using `assign`.
 * Support for iterating the elements of this structure can be added as well.
 */
template <typename T>
class PerThreadVector {
public:
  /// @brief The data structure storing the data
  galois::DistArray<pando::Vector<T>> m_data;

private:
  uint64_t coreY;
  uint64_t cores;
  uint64_t threads;
  uint64_t hosts;

  uint64_t getLocalVectorID() {
    uint64_t coreID = (pando::getCurrentPlace().core.x * coreY) + pando::getCurrentPlace().core.y;
    return ((pando::getCurrentPlace().node.id * cores * threads) + (coreID * threads) +
            pando::getCurrentThread().id);
  }

public:
  friend PTVectorIterator<T>;

  using iterator = PTVectorIterator<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using pointer = pando::GlobalPtr<pando::Vector<T>>;

  PerThreadVector() = default;

  /**
   * @brief Initializes the PerThreadVector array.
   */
  [[nodiscard]] pando::Status initialize() {
    if (m_data.m_data.data() != nullptr) {
      return pando::Status::AlreadyInit;
    }

    coreY = pando::getPlaceDims().core.y;
    cores = pando::getPlaceDims().core.x * coreY;
    threads = pando::getThreadDims().id;
    hosts = pando::getPlaceDims().node.id;

    pando::Status err = m_data.initialize(hosts * cores * threads);
    if (err != pando::Status::Success) {
      return err;
    }

    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      pando::Vector<T> vec;
      err = vec.initialize(0);
      if (err != pando::Status::Success) {
        return err;
      }
      m_data[i] = vec;
    }
    return err;
  }

  /**
   * @brief Deinitializes the PerThreadVector array.
   */
  void deinitialize() {
    if (m_data.m_data.data() == nullptr) {
      return;
    }
    for (pando::Vector<T> vec : m_data) {
      vec.deinitialize();
    }
    m_data.deinitialize();
  }

  /**
   * @brief Returns the current hardware thread's vector.
   */
  pando::GlobalRef<pando::Vector<T>> getThreadVector() {
    return m_data[getLocalVectorID()];
  }

  /**
   * @brief Returns a hardware thread's vector.
   */
  pando::GlobalPtr<pando::Vector<T>> get(size_t i) noexcept {
    return &(m_data[i]);
  }

  /**
   * @brief Returns a hardware thread's vector.
   */
  pando::GlobalPtr<const pando::Vector<T>> get(size_t i) const noexcept {
    return &(m_data[i]);
  }

  /**
   * @brief Appends to the current hardware thread's vector.
   */
  [[nodiscard]] pando::Status pushBack(T val) {
    pando::Vector<T> copy = m_data[getLocalVectorID()];
    pando::Status err = copy.pushBack(val);
    if (err != pando::Status::Success) {
      return err;
    }
    set(copy);
    return err;
  }

  /**
   * @brief Sets the current hardware thread's vector.
   */
  void set(pando::Vector<T> localVec) {
    m_data[getLocalVectorID()] = localVec;
  }

  /**
   * @brief Returns the total number of elements in the PerThreadVector
   */
  size_t sizeAll() const {
    size_t size = 0;
    for (uint64_t i = 0; i < m_data.size(); i++) {
      pando::Vector<T> vec = m_data[i];
      size += vec.size();
    }
    return size;
  }

  /**
   * @brief Returns the total number of per thread vectors
   */
  size_t size() const {
    return m_data.size();
  }

  // TODO(AdityaAtulTewari) Whenever it is time for performance counters they need to be encoded
  // properly
  /**
   * @brief Copies data from one host's PerThreadVector to another as a regular Vector
   *
   * @note Super useful for doing bulk data transfers from remote sources
   * @warning Assumes that the vector "to" is not initialized.
   * @warning Will allocate memory in local main memory
   *
   * @param to this is the vector we are copying to
   */
  [[nodiscard]] pando::Status assign(pando::GlobalPtr<pando::Vector<T>> to) {
    pando::Vector<T> tto = *to;
    auto err = tto.initialize(sizeAll());
    if (err != pando::Status::Success) {
      return err;
    }
    uint64_t pos = 0;
    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      pando::Vector<T> localVec = m_data[i];
      for (uint64_t j = 0; j < localVec.size(); j++) {
        tto[pos] = localVec[j];
      }
    }
    *to = tto;
    return pando::Status::Success;
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
};

/**
 * @brief an iterator that stores the PerThreadVector and the current position to provide random
 * access iterator semantics
 */
template <typename T>
class PTVectorIterator {
  PerThreadVector<T> m_arr;
  std::uint64_t m_pos;
  friend CountedIterator<PTVectorIterator<T>>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = pando::Vector<T>;
  using pointer = pando::GlobalPtr<pando::Vector<T>>;
  using reference = pando::GlobalRef<pando::Vector<T>>;

  PTVectorIterator(PerThreadVector<T> arr, std::uint64_t pos) : m_arr(arr), m_pos(pos) {}

  constexpr PTVectorIterator() noexcept = default;
  constexpr PTVectorIterator(PTVectorIterator&&) noexcept = default;
  constexpr PTVectorIterator(const PTVectorIterator&) noexcept = default;
  ~PTVectorIterator() = default;

  constexpr PTVectorIterator& operator=(const PTVectorIterator&) noexcept = default;
  constexpr PTVectorIterator& operator=(PTVectorIterator&&) noexcept = default;

  reference operator*() const noexcept {
    return *m_arr.get(m_pos);
  }

  reference operator*() noexcept {
    return *m_arr.get(m_pos);
  }

  pointer operator->() {
    return m_arr.get(m_pos);
  }

  PTVectorIterator& operator++() {
    m_pos++;
    return *this;
  }

  PTVectorIterator operator++(int) {
    PTVectorIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  PTVectorIterator& operator--() {
    m_pos--;
    return *this;
  }

  PTVectorIterator operator--(int) {
    PTVectorIterator tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr PTVectorIterator operator+(std::uint64_t n) const noexcept {
    return PTVectorIterator(m_arr, m_pos + n);
  }

  constexpr PTVectorIterator operator-(std::uint64_t n) const noexcept {
    return PTVectorIterator(m_arr, m_pos - n);
  }

  constexpr difference_type operator-(PTVectorIterator b) const noexcept {
    return m_pos - b.m_pos;
  }

  reference operator[](std::uint64_t n) noexcept {
    return m_arr.get(m_pos + n);
  }

  reference operator[](std::uint64_t n) const noexcept {
    return m_arr.get(m_pos + n);
  }

  friend bool operator==(const PTVectorIterator& a, const PTVectorIterator& b) {
    return a.m_pos == b.m_pos && a.m_arr.size() == b.m_arr.size() &&
           a.m_arr.m_data.m_data.data() == b.m_arr.m_data.m_data.data();
  }

  friend bool operator!=(const PTVectorIterator& a, const PTVectorIterator& b) {
    return !(a == b);
  }

  friend bool operator<(const PTVectorIterator& a, const PTVectorIterator& b) {
    return a.m_pos < b.m_pos;
  }

  friend bool operator<=(const PTVectorIterator& a, const PTVectorIterator& b) {
    return a.m_pos <= b.m_pos;
  }

  friend bool operator>(const PTVectorIterator& a, const PTVectorIterator& b) {
    return a.m_pos > b.m_pos;
  }

  friend bool operator>=(const PTVectorIterator& a, const PTVectorIterator& b) {
    return a.m_pos >= b.m_pos;
  }

  friend pando::Place localityOf(PTVectorIterator& a) {
    return pando::localityOf(a.m_arr.get(a.m_pos));
  }
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_
