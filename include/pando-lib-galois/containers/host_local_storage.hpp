// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_STORAGE_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_STORAGE_HPP_

#include <pando-rt/export.h>

#include <utility>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/specific_storage.hpp>
#include <pando-rt/utility/expected.hpp>

namespace galois {

namespace HostLocalStorageHeap {

constexpr std::uint64_t Size = 1 << 25;
constexpr std::uint64_t Granule = 128;
struct ModestArray {
  std::byte arr[Size];
};

extern pando::NodeSpecificStorage<ModestArray> heap;
extern pando::SlabMemoryResource<Granule>* LocalHeapSlab;

void HeapInit();

template <typename T>
[[nodiscard]] pando::Expected<pando::NodeSpecificStorageAlias<T>> allocate() {
  auto ptr = LocalHeapSlab->allocate(sizeof(T));
  if (ptr == nullptr) {
    return pando::Status::BadAlloc;
  }
  pando::GlobalPtr<T> ptrT = static_cast<pando::GlobalPtr<T>>(ptr);
  auto heapAlias = pando::NodeSpecificStorageAlias<ModestArray>(heap);
  return heapAlias.getStorageAliasAt(ptrT);
}

template <typename T>
void deallocate(pando::NodeSpecificStorageAlias<T> toDeAlloc) {
  auto size = sizeof(T);
  auto ptrStartTyped = toDeAlloc.getPointerAt(pando::NodeIndex{0});
  pando::GlobalPtr<void> ptrStartVoid = static_cast<pando::GlobalPtr<void>>(ptrStartTyped);
  LocalHeapSlab->deallocate(ptrStartVoid, size);
}
} // namespace HostLocalStorageHeap

template <typename T>
class HostLocalStorageIt;

template <typename T>
class HostLocalStorage {
  pando::NodeSpecificStorageAlias<T> m_items{};

public:
  HostLocalStorage() noexcept = default;
  HostLocalStorage(const HostLocalStorage&) = default;
  HostLocalStorage(HostLocalStorage&&) = default;

  ~HostLocalStorage() = default;

  HostLocalStorage& operator=(const HostLocalStorage&) = default;
  HostLocalStorage& operator=(HostLocalStorage&&) = default;

  using iterator = HostLocalStorageIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  [[nodiscard]] static constexpr std::uint64_t getNumHosts() noexcept {
    return static_cast<std::uint64_t>(pando::getPlaceDims().node.id);
  }

  [[nodiscard]] static constexpr std::uint64_t getCurrentHost() noexcept {
    return static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
  }

  static constexpr std::uint64_t size() noexcept {
    return getNumHosts();
  }

  [[nodiscard]] pando::Status initialize() {
    m_items = PANDO_EXPECT_RETURN(HostLocalStorageHeap::allocate<T>());
    return pando::Status::Success;
  }

  void deinitialize() {
    HostLocalStorageHeap::deallocate(m_items);
  }

  pando::GlobalPtr<T> getLocal() noexcept {
    return m_items.getPointer();
  }

  pando::GlobalPtr<const T> getLocal() const noexcept {
    return m_items.getPointer();
  }

  pando::GlobalRef<T> getLocalRef() noexcept {
    return *m_items.getPointer();
  }

  pando::GlobalRef<const T> getLocalRef() const noexcept {
    return *m_items.getPointer();
  }

  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    return m_items.getPointerAt(pando::NodeIndex(static_cast<std::int16_t>(i)));
  }

  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    return m_items.getPointerAt(pando::NodeIndex(static_cast<std::int16_t>(i)));
  }

  pando::GlobalRef<T> operator[](std::uint64_t i) noexcept {
    return *this->get(i);
  }

  pando::GlobalRef<const T> operator[](std::uint64_t i) const noexcept {
    return *this->get(i);
  }

  template <typename Y>
  pando::GlobalRef<T> getFromPtr(pando::GlobalPtr<Y> ptr) {
    std::uint64_t i = static_cast<std::uint64_t>(pando::localityOf(ptr).node.id);
    return this->get(i);
  }

  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  iterator end() noexcept {
    return iterator(*this, getNumHosts());
  }

  iterator end() const noexcept {
    return iterator(*this, getNumHosts());
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

  friend bool operator==(const HostLocalStorage& a, const HostLocalStorage& b) {
    return a.m_items == b.m_items;
  }

  friend bool operator!=(const HostLocalStorage& a, const HostLocalStorage& b) {
    return !(a == b);
  }
};

template <typename T>
class HostLocalStorageIt {
  HostLocalStorage<T> m_curr;
  std::int16_t m_loc;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int16_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  HostLocalStorageIt(HostLocalStorage<T> curr, std::int16_t loc) : m_curr(curr), m_loc(loc) {}

  constexpr HostLocalStorageIt() noexcept = default;
  constexpr HostLocalStorageIt(HostLocalStorageIt&&) noexcept = default;
  constexpr HostLocalStorageIt(const HostLocalStorageIt&) noexcept = default;
  ~HostLocalStorageIt() = default;

  constexpr HostLocalStorageIt& operator=(const HostLocalStorageIt&) noexcept = default;
  constexpr HostLocalStorageIt& operator=(HostLocalStorageIt&&) noexcept = default;

  reference operator*() const noexcept {
    return m_curr[m_loc];
  }

  reference operator*() noexcept {
    return m_curr[m_loc];
  }

  pointer operator->() {
    return m_curr.get(m_loc);
  }

  HostLocalStorageIt& operator++() {
    m_loc++;
    return *this;
  }

  HostLocalStorageIt operator++(int) {
    HostLocalStorageIt tmp = *this;
    ++(*this);
    return tmp;
  }

  HostLocalStorageIt& operator--() {
    m_loc--;
    return *this;
  }

  HostLocalStorageIt operator--(int) {
    HostLocalStorageIt tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr HostLocalStorageIt operator+(std::uint64_t n) const noexcept {
    return HostLocalStorageIt(m_curr, m_loc + n);
  }

  constexpr HostLocalStorageIt& operator+=(std::uint64_t n) noexcept {
    m_loc += n;
    return *this;
  }

  constexpr HostLocalStorageIt operator-(std::uint64_t n) const noexcept {
    return HostLocalStorageIt(m_curr, m_loc - n);
  }

  constexpr difference_type operator-(HostLocalStorageIt b) const noexcept {
    return m_loc - b.loc;
  }

  friend bool operator==(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return a.m_loc == b.m_loc;
  }

  friend bool operator!=(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return !(a == b);
  }

  friend bool operator<(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return a.m_loc < b.m_loc;
  }

  friend bool operator<=(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return a.m_loc <= b.m_loc;
  }

  friend bool operator>(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return a.m_loc > b.m_loc;
  }

  friend bool operator>=(const HostLocalStorageIt& a, const HostLocalStorageIt& b) {
    return a.m_loc >= b.m_loc;
  }

  friend pando::Place localityOf(HostLocalStorageIt& a) {
    return pando::Place{pando::NodeIndex{a.m_loc}, pando::anyPod, pando::anyCore};
  }
};

template <typename T>
[[nodiscard]] pando::Expected<galois::HostLocalStorage<T>> copyToAllHosts(T&& cont) {
  galois::HostLocalStorage<T> ret{};
  PANDO_CHECK_RETURN(ret.initialize());
  PANDO_CHECK_RETURN(galois::doAll(
      cont, ret, +[](T cont, pando::GlobalRef<T> refcopy) {
        T copy;
        if (galois::localityOf(cont).node.id != pando::getCurrentPlace().node.id) {
          const std::uint64_t size = cont.size();
          PANDO_CHECK(copy.initialize(size));
          for (std::uint64_t i = 0; i < cont.size(); i++) {
            copy.get(i) = cont.get(i);
          }
        } else {
          copy = cont;
        }
        refcopy = copy;
      }));
  return ret;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_STORAGE_HPP_
