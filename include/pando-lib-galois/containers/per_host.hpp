// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_PER_HOST_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_PER_HOST_HPP_

#include <pando-rt/export.h>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/allocate_memory.hpp>

namespace galois {

template <typename T>
class PerHostIt;

template <typename T>
class PerHost {
  pando::GlobalPtr<T> m_items;

public:
  PerHost() noexcept = default;
  PerHost(const PerHost&) = default;
  PerHost(PerHost&&) = default;

  ~PerHost() = default;

  PerHost& operator=(const PerHost&) = default;
  PerHost& operator=(PerHost&&) = default;

  using iterator = PerHostIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  [[nodiscard]] constexpr std::uint64_t getNumHosts() const noexcept {
    return static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  }

  [[nodiscard]] constexpr std::uint64_t getCurrentNode() const noexcept {
    return static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
  }

  std::uint64_t size() {
    return getNumHosts();
  }

  [[nodiscard]] pando::Status initialize() {
    auto expect =
        pando::allocateMemory<T>(getNumHosts(), pando::getCurrentPlace(), pando::MemoryType::Main);
    if (!expect.hasValue()) {
      return expect.error();
    }
    m_items = expect.value();
    return pando::Status::Success;
  }

  void deinitialize() {
    deallocateMemory(m_items, getNumHosts());
  }

  pando::GlobalRef<T> getLocal() noexcept {
    return m_items[getCurrentNode()];
  }

  pando::GlobalRef<T> get(std::uint64_t i) noexcept {
    return m_items[i];
  }

  template <typename Y>
  pando::GlobalRef<T> getFromPtr(pando::GlobalPtr<Y> ptr) {
    std::uint64_t i = static_cast<std::uint64_t>(pando::localityOf(ptr).node.id);
    return this->get(i);
  }

  iterator begin() noexcept {
    return iterator(m_items, 0);
  }

  iterator begin() const noexcept {
    return iterator(m_items, 0);
  }

  iterator end() noexcept {
    return iterator(m_items + getNumHosts(), getNumHosts());
  }

  iterator end() const noexcept {
    return iterator(m_items + getNumHosts(), getNumHosts());
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

template <typename T>
class PerHostIt {
  pando::GlobalPtr<T> m_curr;
  std::int16_t m_loc;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int16_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  PerHostIt(pando::GlobalPtr<T> curr, std::int16_t loc) : m_curr(curr), m_loc(loc) {}

  constexpr PerHostIt() noexcept = default;
  constexpr PerHostIt(PerHostIt&&) noexcept = default;
  constexpr PerHostIt(const PerHostIt&) noexcept = default;
  ~PerHostIt() = default;

  constexpr PerHostIt& operator=(const PerHostIt&) noexcept = default;
  constexpr PerHostIt& operator=(PerHostIt&&) noexcept = default;

  reference operator*() const noexcept {
    return *m_curr;
  }

  reference operator*() noexcept {
    return *m_curr;
  }

  pointer operator->() {
    return m_curr;
  }

  PerHostIt& operator++() {
    m_curr++;
    m_loc++;
    return *this;
  }

  PerHostIt operator++(int) {
    PerHostIt tmp = *this;
    ++(*this);
    return tmp;
  }

  PerHostIt& operator--() {
    m_curr--;
    m_loc--;
    return *this;
  }

  PerHostIt operator--(int) {
    PerHostIt tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr PerHostIt operator+(std::uint64_t n) const noexcept {
    return PerHostIt(m_curr + n, m_loc + n);
  }

  constexpr PerHostIt& operator+=(std::uint64_t n) noexcept {
    m_curr += n;
    m_loc += n;
    return *this;
  }

  constexpr PerHostIt operator-(std::uint64_t n) const noexcept {
    return PerHostIt(m_curr - n, m_loc - n);
  }

  constexpr difference_type operator-(PerHostIt b) const noexcept {
    return m_loc - b.loc;
  }

  friend bool operator==(const PerHostIt& a, const PerHostIt& b) {
    return a.m_curr == b.m_curr;
  }

  friend bool operator!=(const PerHostIt& a, const PerHostIt& b) {
    return !(a == b);
  }

  friend bool operator<(const PerHostIt& a, const PerHostIt& b) {
    return a.m_curr < b.m_curr;
  }

  friend bool operator<=(const PerHostIt& a, const PerHostIt& b) {
    return a.m_curr <= b.m_curr;
  }

  friend bool operator>(const PerHostIt& a, const PerHostIt& b) {
    return a.m_curr > b.m_curr;
  }

  friend bool operator>=(const PerHostIt& a, const PerHostIt& b) {
    return a.m_curr >= b.m_curr;
  }

  friend pando::Place localityOf(PerHostIt& a) {
    return pando::Place{pando::NodeIndex{a.m_loc}, pando::anyPod, pando::anyCore};
  }
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_PER_HOST_HPP_
