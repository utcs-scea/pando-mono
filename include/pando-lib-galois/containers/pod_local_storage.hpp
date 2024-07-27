// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023-2024. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_POD_LOCAL_STORAGE_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_POD_LOCAL_STORAGE_HPP_

#include <pando-rt/export.h>

#include <utility>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/specific_storage.hpp>
#include <pando-rt/utility/expected.hpp>

namespace galois {

namespace PodLocalStorageHeap {

constexpr std::uint64_t Size = 1 << 10;
constexpr std::uint64_t Granule = 128;
struct ModestArray {
  std::byte arr[Size];
};

extern pando::PodSpecificStorage<ModestArray> heap;
extern pando::SlabMemoryResource<Granule>* LocalHeapSlab;

void HeapInit();

template <typename T>
[[nodiscard]] pando::Expected<pando::PodSpecificStorageAlias<T>> allocate() {
  auto ptr = LocalHeapSlab->allocate(sizeof(T));
  if (ptr == nullptr) {
    return pando::Status::BadAlloc;
  }
  pando::GlobalPtr<T> ptrT = static_cast<pando::GlobalPtr<T>>(ptr);
  auto heapAlias = pando::PodSpecificStorageAlias<ModestArray>(heap);
  return heapAlias.getStorageAliasAt(ptrT);
}

template <typename T>
void deallocate(pando::PodSpecificStorageAlias<T> toDeAlloc) {
  auto size = sizeof(T);
  auto ptrStartTyped = toDeAlloc.getPointerAt(pando::NodeIndex{0}, pando::PodIndex{0, 0});
  pando::GlobalPtr<void> ptrStartVoid = static_cast<pando::GlobalPtr<void>>(ptrStartTyped);
  LocalHeapSlab->deallocate(ptrStartVoid, size);
}
} // namespace PodLocalStorageHeap

template <typename T>
class PodLocalStorageIt;

template <typename T>
class PodLocalStorage {
  pando::PodSpecificStorageAlias<T> m_items{};

public:
  PodLocalStorage() noexcept = default;
  PodLocalStorage(const PodLocalStorage&) = default;
  PodLocalStorage(PodLocalStorage&&) = default;

  ~PodLocalStorage() = default;

  PodLocalStorage& operator=(const PodLocalStorage&) = default;
  PodLocalStorage& operator=(PodLocalStorage&&) = default;

  using iterator = PodLocalStorageIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  [[nodiscard]] static constexpr std::uint64_t getNumPods() noexcept {
    const auto p = pando::getPlaceDims();
    return static_cast<std::uint64_t>(p.node.id * p.pod.x * p.pod.y);
  }

  [[nodiscard]] static constexpr std::uint64_t getCurrentPodIdx() noexcept {
    const auto dim = pando::getPlaceDims();
    const auto cur = pando::getCurrentPlace();
    return static_cast<std::uint64_t>(cur.node.id * dim.pod.x * dim.pod.y + cur.pod.x * dim.pod.y +
                                      cur.pod.y);
  }

  [[nodiscard]] static constexpr pando::Place getPlaceFromPodIdx(std::uint64_t idx) noexcept {
    const auto dim = pando::getPlaceDims();
    const auto pods = dim.pod.x * dim.pod.y;
    const pando::NodeIndex node = pando::NodeIndex(idx / pods);
    const std::int16_t localPodIdx = idx % pods;
    const pando::PodIndex pod = pando::PodIndex(localPodIdx / dim.pod.x, localPodIdx % dim.pod.x);
    return pando::Place(node, pod, pando::anyCore);
  }

  static constexpr std::uint64_t size() noexcept {
    return getNumPods();
  }

  [[nodiscard]] pando::Status initialize() {
    m_items = PANDO_EXPECT_RETURN(PodLocalStorageHeap::allocate<T>());
    return pando::Status::Success;
  }

  void deinitialize() {
    PodLocalStorageHeap::deallocate(m_items);
  }

  pando::GlobalRef<T> getLocal() noexcept {
    return *m_items.getPointer();
  }

  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    auto place = getPlaceFromPodIdx(i);
    return *m_items.getPointerAt(place.node, place.pod);
  }

  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    auto place = getPlaceFromPodIdx(i);
    return m_items.getPointerAt(place.node, place.pod);
  }

  pando::GlobalRef<T> operator[](std::uint64_t i) noexcept {
    auto place = getPlaceFromPodIdx(i);
    return *m_items.getPointerAt(place.node, place.pod);
  }

  pando::GlobalRef<const T> operator[](std::uint64_t i) const noexcept {
    auto place = getPlaceFromPodIdx(i);
    return *m_items.getPointerAt(place.node, place.pod);
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
    return iterator(*this, getNumPods());
  }

  iterator end() const noexcept {
    return iterator(*this, getNumPods());
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

  friend bool operator==(const PodLocalStorage& a, const PodLocalStorage& b) {
    const pando::NodeIndex node(0);
    const pando::PodIndex pod(0, 0);
    return a.m_items.getPointerAt(node, pod) == b.m_items.getPointerAt(node, pod);
  }

  friend bool operator!=(const PodLocalStorage& a, const PodLocalStorage& b) {
    return !(a == b);
  }
};

template <typename T>
class PodLocalStorageIt {
  PodLocalStorage<T> m_curr;
  std::uint64_t m_loc;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  PodLocalStorageIt(PodLocalStorage<T> curr, std::uint64_t loc) : m_curr(curr), m_loc(loc) {}

  constexpr PodLocalStorageIt() noexcept = default;
  constexpr PodLocalStorageIt(PodLocalStorageIt&&) noexcept = default;
  constexpr PodLocalStorageIt(const PodLocalStorageIt&) noexcept = default;
  ~PodLocalStorageIt() = default;

  constexpr PodLocalStorageIt& operator=(const PodLocalStorageIt&) noexcept = default;
  constexpr PodLocalStorageIt& operator=(PodLocalStorageIt&&) noexcept = default;

  reference operator*() const noexcept {
    return m_curr[m_loc];
  }

  reference operator*() noexcept {
    return m_curr[m_loc];
  }

  pointer operator->() {
    return m_curr.get(m_loc);
  }

  PodLocalStorageIt& operator++() {
    m_loc++;
    return *this;
  }

  PodLocalStorageIt operator++(int) {
    PodLocalStorageIt tmp = *this;
    ++(*this);
    return tmp;
  }

  PodLocalStorageIt& operator--() {
    m_loc--;
    return *this;
  }

  PodLocalStorageIt operator--(int) {
    PodLocalStorageIt tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr PodLocalStorageIt operator+(std::uint64_t n) const noexcept {
    return PodLocalStorageIt(m_curr, m_loc + n);
  }

  constexpr PodLocalStorageIt& operator+=(std::uint64_t n) noexcept {
    m_loc += n;
    return *this;
  }

  constexpr PodLocalStorageIt operator-(std::uint64_t n) const noexcept {
    return PodLocalStorageIt(m_curr, m_loc - n);
  }

  constexpr difference_type operator-(PodLocalStorageIt b) const noexcept {
    return m_loc - b.loc;
  }

  friend bool operator==(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return a.m_loc == b.m_loc;
  }

  friend bool operator!=(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return !(a == b);
  }

  friend bool operator<(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return a.m_loc < b.m_loc;
  }

  friend bool operator<=(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return a.m_loc <= b.m_loc;
  }

  friend bool operator>(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return a.m_loc > b.m_loc;
  }

  friend bool operator>=(const PodLocalStorageIt& a, const PodLocalStorageIt& b) {
    return a.m_loc >= b.m_loc;
  }

  friend pando::Place localityOf(PodLocalStorageIt& a) {
    return a.m_curr.getPlaceFromPodIdx(a.m_loc);
  }
};

template <typename T>
[[nodiscard]] pando::Expected<galois::PodLocalStorage<T>> copyToAllPods(T& cont) {
  galois::PodLocalStorage<T> ret{};
  PANDO_CHECK_RETURN(ret.initialize());
  PANDO_CHECK_RETURN(galois::doAllExplicitPolicy<SchedulerPolicy::INFER_RANDOM_CORE>(
      cont, ret, +[](T cont, pando::GlobalRef<T> refcopy) {
        T copy;
        const std::uint64_t size = cont.size();
        PANDO_CHECK(copy.initialize(size));
        for (std::uint64_t i = 0; i < cont.size(); i++) {
          copy[i] = cont[i];
        }
        refcopy = copy;
      }));
  return ret;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_POD_LOCAL_STORAGE_HPP_
