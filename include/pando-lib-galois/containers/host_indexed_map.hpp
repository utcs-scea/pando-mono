// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_HOST_INDEXED_MAP_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_HOST_INDEXED_MAP_HPP_

#include <pando-rt/export.h>

#include <cassert>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/allocate_memory.hpp>

namespace galois {

template <typename T>
class HostIndexedMapIt;

template <typename T>
class HostIndexedMap {
  pando::GlobalPtr<T> m_items;

public:
  HostIndexedMap() noexcept = default;
  HostIndexedMap(const HostIndexedMap&) = default;
  HostIndexedMap(HostIndexedMap&&) = default;

  ~HostIndexedMap() = default;

  HostIndexedMap& operator=(const HostIndexedMap&) = default;
  HostIndexedMap& operator=(HostIndexedMap&&) = default;

  using iterator = HostIndexedMapIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  [[nodiscard]] static constexpr std::uint64_t getNumHosts() noexcept {
    return static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  }

  [[nodiscard]] constexpr std::uint64_t getCurrentHost() const noexcept {
    return static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
  }

  static constexpr std::uint64_t size() noexcept {
    return getNumHosts();
  }

  [[nodiscard]] pando::Status initialize(pando::Place place, pando::MemoryType memory) {
    auto expect = pando::allocateMemory<T>(getNumHosts(), place, memory);
    if (!expect.hasValue()) {
      return expect.error();
    }
    m_items = expect.value();
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status initialize() {
    return initialize(pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  [[nodiscard]] pando::Status initialize([[maybe_unused]] std::uint64_t numNodes) {
    assert(size() == numNodes);
    return initialize();
  }

  [[nodiscard]] pando::Status initialize([[maybe_unused]] std::uint64_t numNodes,
                                         pando::Place place, pando::MemoryType memory) {
    assert(size() == numNodes);
    return initialize(place, memory);
  }

  void deinitialize() {
    deallocateMemory(m_items, getNumHosts());
  }

  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    return &m_items[i];
  }

  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    return &m_items[i];
  }

  pando::GlobalPtr<T> getLocal() noexcept {
    return &m_items[getCurrentHost()];
  }

  pando::GlobalPtr<const T> getLocal() const noexcept {
    return &m_items[getCurrentHost()];
  }

  pando::GlobalRef<T> getLocalRef() noexcept {
    return m_items[getCurrentHost()];
  }

  pando::GlobalRef<const T> getLocalRef() const noexcept {
    return m_items[getCurrentHost()];
  }

  pando::GlobalRef<T> operator[](std::uint64_t i) noexcept {
    return *this->get(i);
  }

  pando::GlobalRef<const T> operator[](std::uint64_t i) const noexcept {
    return *this->get(i);
  }

  template <typename Y>
  pando::GlobalPtr<T> getFromPtr(pando::GlobalPtr<Y> ptr) {
    std::uint64_t i = static_cast<std::uint64_t>(pando::localityOf(ptr).node.id);
    return this->get(i);
  }

  template <typename Y>
  pando::GlobalRef<T> getRefFromPtr(pando::GlobalPtr<Y> ptr) {
    return *getFromPtr(ptr);
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

  friend bool operator==(const HostIndexedMap& a, const HostIndexedMap& b) {
    return a.m_items == b.m_items;
  }

  friend bool operator!=(const HostIndexedMap& a, const HostIndexedMap& b) {
    return !(a == b);
  }

  friend pando::Place localityOf(HostIndexedMap& a) {
    return pando::localityOf(a.m_items);
  }
};

template <typename T>
class HostIndexedMapIt {
  pando::GlobalPtr<T> m_curr;
  std::int16_t m_loc;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int16_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  HostIndexedMapIt(pando::GlobalPtr<T> curr, std::int16_t loc) : m_curr(curr), m_loc(loc) {}

  constexpr HostIndexedMapIt() noexcept = default;
  constexpr HostIndexedMapIt(HostIndexedMapIt&&) noexcept = default;
  constexpr HostIndexedMapIt(const HostIndexedMapIt&) noexcept = default;
  ~HostIndexedMapIt() = default;

  constexpr HostIndexedMapIt& operator=(const HostIndexedMapIt&) noexcept = default;
  constexpr HostIndexedMapIt& operator=(HostIndexedMapIt&&) noexcept = default;

  reference operator*() const noexcept {
    return *m_curr;
  }

  reference operator*() noexcept {
    return *m_curr;
  }

  pointer operator->() {
    return m_curr;
  }

  HostIndexedMapIt& operator++() {
    m_curr++;
    m_loc++;
    return *this;
  }

  HostIndexedMapIt operator++(int) {
    HostIndexedMapIt tmp = *this;
    ++(*this);
    return tmp;
  }

  HostIndexedMapIt& operator--() {
    m_curr--;
    m_loc--;
    return *this;
  }

  HostIndexedMapIt operator--(int) {
    HostIndexedMapIt tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr HostIndexedMapIt operator+(std::uint64_t n) const noexcept {
    return HostIndexedMapIt(m_curr + n, m_loc + n);
  }

  constexpr HostIndexedMapIt& operator+=(std::uint64_t n) noexcept {
    m_curr += n;
    m_loc += n;
    return *this;
  }

  constexpr HostIndexedMapIt operator-(std::uint64_t n) const noexcept {
    return HostIndexedMapIt(m_curr - n, m_loc - n);
  }

  constexpr difference_type operator-(HostIndexedMapIt b) const noexcept {
    return m_loc - b.loc;
  }

  friend bool operator==(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return a.m_curr == b.m_curr;
  }

  friend bool operator!=(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return !(a == b);
  }

  friend bool operator<(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return a.m_curr < b.m_curr;
  }

  friend bool operator<=(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return a.m_curr <= b.m_curr;
  }

  friend bool operator>(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return a.m_curr > b.m_curr;
  }

  friend bool operator>=(const HostIndexedMapIt& a, const HostIndexedMapIt& b) {
    return a.m_curr >= b.m_curr;
  }

  friend pando::Place localityOf(HostIndexedMapIt& a) {
    return pando::Place{pando::NodeIndex{a.m_loc}, pando::anyPod, pando::anyCore};
  }
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_HOST_INDEXED_MAP_HPP_
