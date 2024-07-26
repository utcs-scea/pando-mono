// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2024. University of Texas at Austin. All rights reserved. */

#ifndef PANDO_LIB_GALOIS_CONTAINERS_INNER_VECTOR_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_INNER_VECTOR_HPP_

#include <utility>

#include "pando-rt/export.h"

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>

namespace galois {

template <typename T>
class GlobalPtrInnerLocality {
  pando::GlobalPtr<T> m_ptr;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  explicit GlobalPtrInnerLocality(pando::GlobalPtr<T> ptr) : m_ptr(ptr) {}

  constexpr GlobalPtrInnerLocality() noexcept = default;
  constexpr GlobalPtrInnerLocality(GlobalPtrInnerLocality&&) noexcept = default;
  constexpr GlobalPtrInnerLocality(const GlobalPtrInnerLocality&) noexcept = default;
  ~GlobalPtrInnerLocality() = default;

  constexpr GlobalPtrInnerLocality& operator=(const GlobalPtrInnerLocality&) noexcept = default;
  constexpr GlobalPtrInnerLocality& operator=(GlobalPtrInnerLocality&&) noexcept = default;

  reference operator*() const noexcept {
    return *m_ptr;
  }

  reference operator*() noexcept {
    return *m_ptr;
  }

  pointer operator->() {
    return m_ptr;
  }

  GlobalPtrInnerLocality& operator++() {
    m_ptr++;
    return *this;
  }

  GlobalPtrInnerLocality operator++(int) {
    GlobalPtrInnerLocality tmp = *this;
    ++(*this);
    return tmp;
  }

  GlobalPtrInnerLocality& operator--() {
    m_ptr--;
    return *this;
  }

  GlobalPtrInnerLocality operator--(int) {
    GlobalPtrInnerLocality tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr GlobalPtrInnerLocality operator+(std::uint64_t n) const noexcept {
    return GlobalPtrInnerLocality(m_ptr + n);
  }

  constexpr GlobalPtrInnerLocality& operator+=(std::uint64_t n) noexcept {
    m_ptr += n;
    return *this;
  }

  constexpr GlobalPtrInnerLocality operator-(std::uint64_t n) const noexcept {
    return GlobalPtrInnerLocality(m_ptr - n);
  }

  constexpr difference_type operator-(GlobalPtrInnerLocality b) const noexcept {
    return m_ptr - b.m_ptr;
  }

  reference operator[](std::uint64_t n) noexcept {
    return m_ptr[n];
  }

  reference operator[](std::uint64_t n) const noexcept {
    return m_ptr[n];
  }

  friend bool operator==(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return a.m_ptr == b.m_ptr;
  }

  friend bool operator!=(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return !(a == b);
  }

  friend bool operator<(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return a.m_ptr < b.m_ptr;
  }

  friend bool operator<=(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return a.m_ptr <= b.m_ptr;
  }

  friend bool operator>(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return a.m_ptr > b.m_ptr;
  }

  friend bool operator>=(const GlobalPtrInnerLocality& a, const GlobalPtrInnerLocality& b) {
    return a.m_ptr >= b.m_ptr;
  }

  friend pando::Place localityOf(GlobalPtrInnerLocality& a) {
    T val = *a.m_ptr;
    return galois::localityOf(val);
  }
};

template <typename T>
class InnerVector {
  pando::Vector<T> m_vec;

public:
  using iterator = galois::GlobalPtrInnerLocality<T>;
  using const_iterator = galois::GlobalPtrInnerLocality<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit constexpr InnerVector(pando::Vector<T>&& v) : m_vec(std::move(v)) {}

  constexpr InnerVector() noexcept = default;

  constexpr InnerVector(InnerVector&&) noexcept = default;
  constexpr InnerVector(const InnerVector&) noexcept = default;

  ~InnerVector() = default;

  constexpr InnerVector& operator=(const InnerVector&) noexcept = default;
  constexpr InnerVector& operator=(InnerVector&&) noexcept = default;

  [[nodiscard]] pando::Status initialize(std::uint64_t size, pando::Place place,
                                         pando::MemoryType memoryType) {
    return m_vec.initialize(size, place, memoryType);
  }

  [[nodiscard]] pando::Status initialize(std::uint64_t size) {
    return m_vec.initialize(size);
  }

  /**
   * @brief Deinitializes the container.
   */
  void deinitialize() {
    m_vec.deinitialize();
  }

  /**
   * @brief Returns the memory this vector is allocated in.
   */
  pando::MemoryType getMemoryType() const noexcept {
    return m_vec.getMemoryType();
  }

  /**
   * @brief Reserves space in the container for at least @p nextCapacity number of elements.
   *
   * @note If the new capacity is less that the current capacity, the function has not effect.
   * @note The function does not change the size of the container and does not initialize the new
   *       elements.
   * @note If the container has not been initialized, then the memory will be @ref MemoryType::Main.
   *
   * @param[in] nextCapacity new capacity of the container in elements
   */
  [[nodiscard]] pando::Status reserve(std::uint64_t nextCapacity) {
    return m_vec.reserve(nextCapacity);
  }

  /**
   * @brief this function resizes the array
   *
   * @note the implementation is simple because T must be trivially copyable.
   *
   * @param[in] newSize the new desired size
   **/
  [[nodiscard]] pando::Status resize(std::uint64_t newSize) {
    return m_vec.resize(newSize);
  }

  /**
   * @brief clear the vector
   */
  void clear() {
    m_vec.clear();
  }

  constexpr std::uint64_t capacity() const noexcept {
    return m_vec.capacity();
  }

  constexpr bool empty() const noexcept {
    return m_vec.empty();
  }

  constexpr auto get(std::uint64_t pos) {
    return m_vec.get(pos);
  }

  constexpr auto operator[](std::uint64_t pos) {
    return m_vec[pos];
  }

  constexpr auto operator[](std::uint64_t pos) const {
    return m_vec[pos];
  }

  constexpr pando::GlobalPtr<T> data() noexcept {
    return m_vec.data();
  }

  constexpr pando::GlobalPtr<const T> data() const noexcept {
    return m_vec.data();
  }

  constexpr std::uint64_t size() const noexcept {
    return m_vec.size();
  }

  /**
   * @brief Appends the element to the end of the vector.
   *
   * @warning If the operation will increase the size of the container past its capacity, a
   *          realocation takes places.
   *
   * @param[in] value element to append
   */
  [[nodiscard]] pando::Status pushBack(const T& value) {
    return m_vec.pushBack(value);
  }

  /**
   * @copydoc pushBack(const T&)
   */
  [[nodiscard]] pando::Status pushBack(T&& value) {
    return m_vec.pushBack(value);
  }

  // TODO(AdityaAtulTewari) Whenever it is time for performance counters they need to be encoded
  // properly
  /**
   * @brief Copies data from one vector to another
   *
   * @note Super useful for doing bulk data transfers from remote sources
   * @warning Assumes that this vector is not initialized.
   * @warning Will allocate memory in local main memory
   *
   * @param from this is the vector we are copying from
   */
  [[nodiscard]] pando::Status assign(pando::GlobalPtr<pando::Vector<T>> from) {
    return m_vec.assign(from);
  }

  // TODO(AdityaAtulTewari) Whenever it is time for performance counters they need to be encoded
  // properly
  /**
   * @brief Copies data from one vector and appends it to another
   *
   * @note Super useful for doing bulk data transfers from remote sources
   *
   * @param from this is the vector we are copying from
   */
  [[nodiscard]] pando::Status append(pando::GlobalPtr<pando::Vector<T>> from) {
    return m_vec.append(from);
  }

  iterator begin() noexcept {
    return iterator(data());
  }

  const_iterator begin() const noexcept {
    return const_iterator(data());
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(data());
  }

  iterator end() noexcept {
    return iterator(data() + size());
  }

  const_iterator end() const noexcept {
    return const_iterator(data() + size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(data() + size());
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
   * @copydoc rbegin()
   */
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend()--);
  }

  /**
   * reverse iterator to the last element
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

  /**
   * @copydoc rend()
   */
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin()--);
  }
};

/// @ingroup ROOT
template <typename T>
bool operator==(const InnerVector<T>& lhs, const InnerVector<T>& rhs) noexcept {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/// @ingroup ROOT
template <typename T>
bool operator!=(const InnerVector<T>& lhs, const InnerVector<T>& rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_INNER_VECTOR_HPP_
