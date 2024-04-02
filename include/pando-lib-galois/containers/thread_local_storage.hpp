// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_STORAGE_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_STORAGE_HPP_

#include <pando-rt/export.h>

#include <utility>

#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/utility/expected.hpp>

namespace galois {

template <typename T>
class ThreadLocalStorageIt;

template <typename T>
class ThreadLocalStorage {
  galois::PodLocalStorage<pando::GlobalPtr<T>> m_items{};

public:
  ThreadLocalStorage() noexcept = default;
  ThreadLocalStorage(const ThreadLocalStorage&) = default;
  ThreadLocalStorage(ThreadLocalStorage&&) = default;

  ~ThreadLocalStorage() = default;

  ThreadLocalStorage& operator=(const ThreadLocalStorage&) = default;
  ThreadLocalStorage& operator=(ThreadLocalStorage&&) = default;

  using iterator = ThreadLocalStorageIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  [[nodiscard]] static constexpr std::uint64_t getNumThreads() noexcept {
    const auto place = pando::getPlaceDims();
    std::uint64_t nodes = static_cast<std::uint64_t>(place.node.id);
    std::uint64_t pods = static_cast<std::uint64_t>(place.pod.x * place.pod.y);
    std::uint64_t cores = static_cast<std::uint64_t>(place.core.x * place.core.y);
    std::uint64_t threads = static_cast<std::uint64_t>(pando::getThreadDims().id);
    return nodes * pods * cores * threads;
  }

  [[nodiscard]] constexpr std::uint64_t getCurrentThreadIdx() const noexcept {
    return getThreadIdxFromPlace(pando::getCurrentPlace(), pando::getCurrentThread());
  }

  [[nodiscard]] static constexpr std::uint64_t getThreadIdxFromPlace(
      pando::Place place, pando::ThreadIndex thread) noexcept {
    const auto placeDims = pando::getPlaceDims();
    const auto threadDims = pando::getThreadDims();
    const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
    const std::uint64_t threadsPerPod = threadsPerCore *
                                        static_cast<std::uint64_t>(placeDims.core.x) *
                                        static_cast<std::uint64_t>(placeDims.core.y);
    const std::uint64_t threadsPerHost = threadsPerPod *
                                         static_cast<std::uint64_t>(placeDims.pod.x) *
                                         static_cast<std::uint64_t>(placeDims.pod.y);
    const std::uint64_t hostIdx = place.node.id;
    const std::uint64_t podIdx = place.pod.x * placeDims.pod.y + place.pod.y;
    const std::uint64_t coreIdx = place.core.x * placeDims.core.y + place.core.y;
    const std::uint64_t threadIdx = thread.id;
    return hostIdx * threadsPerHost + podIdx * threadsPerPod + coreIdx * threadsPerCore + threadIdx;
  }

  [[nodiscard]] static constexpr galois::Tuple2<pando::Place, pando::ThreadIndex>
  getPlaceFromThreadIdx(std::uint64_t idx) noexcept {
    const auto placeDims = pando::getPlaceDims();
    const auto threadDims = pando::getThreadDims();
    const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
    const std::uint64_t threadsPerPod = threadsPerCore *
                                        static_cast<std::uint64_t>(placeDims.core.x) *
                                        static_cast<std::uint64_t>(placeDims.core.y);
    const std::uint64_t threadsPerHost = threadsPerPod *
                                         static_cast<std::uint64_t>(placeDims.pod.x) *
                                         static_cast<std::uint64_t>(placeDims.pod.y);
    const pando::NodeIndex node(static_cast<int16_t>(idx / threadsPerHost));
    const std::uint64_t threadPerHostIdx = idx % threadsPerHost;
    const std::uint64_t podPerHostIdx = threadPerHostIdx / threadsPerPod;
    const pando::PodIndex pod(podPerHostIdx / placeDims.pod.y, podPerHostIdx % placeDims.pod.y);
    const std::uint64_t threadPerPodIdx = threadPerHostIdx % threadsPerPod;
    const std::uint64_t corePerPodIdx = threadPerPodIdx / threadsPerCore;
    const pando::CoreIndex core(corePerPodIdx / placeDims.core.y, corePerPodIdx % placeDims.core.y);
    const std::uint64_t threadPerCoreIdx = threadPerPodIdx % threadsPerCore;
    const pando::ThreadIndex thread(threadPerCoreIdx);
    return galois::make_tpl(pando::Place{node, pod, core}, thread);
  }

  static constexpr std::uint64_t size() noexcept {
    return getNumThreads();
  }

  [[nodiscard]] pando::Status initialize() {
    PANDO_CHECK_RETURN(m_items.initialize());
    PANDO_CHECK_RETURN(galois::doAll(
        m_items, +[](pando::GlobalRef<pando::GlobalPtr<T>> ref) {
          const auto placeDims = pando::getPlaceDims();
          const auto threadDims = pando::getThreadDims();
          const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
          const std::uint64_t threadsPerPod = threadsPerCore *
                                              static_cast<std::uint64_t>(placeDims.core.x) *
                                              static_cast<std::uint64_t>(placeDims.core.y);
          const auto place = pando::localityOf(&ref);
          ref = PANDO_EXPECT_CHECK(
              pando::allocateMemory<T>(threadsPerPod, place, pando::MemoryType::L2SP));
        }));
    return pando::Status::Success;
  }

  void deinitialize() {
    PANDO_CHECK(galois::doAll(
        m_items, +[](pando::GlobalRef<pando::GlobalPtr<T>> ptr) {
          const auto placeDims = pando::getPlaceDims();
          const auto threadDims = pando::getThreadDims();
          const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
          const std::uint64_t threadsPerPod = threadsPerCore *
                                              static_cast<std::uint64_t>(placeDims.core.x) *
                                              static_cast<std::uint64_t>(placeDims.core.y);
          pando::deallocateMemory<T>(ptr, threadsPerPod);
        }));
    m_items.deinitialize();
  }

  pando::GlobalPtr<T> get(std::uint64_t i) noexcept {
    const auto placeDims = pando::getPlaceDims();
    const auto threadDims = pando::getThreadDims();
    const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
    const std::uint64_t threadsPerPod = threadsPerCore *
                                        static_cast<std::uint64_t>(placeDims.core.x) *
                                        static_cast<std::uint64_t>(placeDims.core.y);
    const std::uint64_t podIdx = i / threadsPerPod;
    const std::uint64_t threadPerPodIdx = i % threadsPerPod;
    pando::GlobalPtr<T> arr = m_items[podIdx];
    return (arr + threadPerPodIdx);
  }

  pando::GlobalPtr<const T> get(std::uint64_t i) const noexcept {
    const auto placeDims = pando::getPlaceDims();
    const auto threadDims = pando::getThreadDims();
    const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
    const std::uint64_t threadsPerPod = threadsPerCore *
                                        static_cast<std::uint64_t>(placeDims.core.x) *
                                        static_cast<std::uint64_t>(placeDims.core.y);
    const std::uint64_t podIdx = i / threadsPerPod;
    const std::uint64_t threadPerPodIdx = i % threadsPerPod;
    pando::GlobalPtr<T> arr = m_items[podIdx];
    return (arr + threadPerPodIdx);
  }

  pando::GlobalRef<T> operator[](std::uint64_t i) noexcept {
    return *this->get(i);
  }

  pando::GlobalRef<const T> operator[](std::uint64_t i) const noexcept {
    return *this->get(i);
  }

  pando::GlobalPtr<T> getLocal() noexcept {
    return this->get(this->getCurrentThreadIdx());
  }

  pando::GlobalRef<T> getLocalRef() noexcept {
    return *this->get(this->getCurrentThreadIdx());
  }

  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  iterator end() noexcept {
    return iterator(*this, getNumThreads());
  }

  iterator end() const noexcept {
    return iterator(*this, getNumThreads());
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

  friend bool operator==(const ThreadLocalStorage& a, const ThreadLocalStorage& b) {
    return a.m_items == b.m_items;
  }

  friend bool operator!=(const ThreadLocalStorage& a, const ThreadLocalStorage& b) {
    return !(a == b);
  }
};

template <typename T>
class ThreadLocalStorageIt {
  ThreadLocalStorage<T> m_curr;
  std::uint64_t m_loc;

public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::int64_t;
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;

  ThreadLocalStorageIt(ThreadLocalStorage<T> curr, std::uint64_t loc) : m_curr(curr), m_loc(loc) {}

  constexpr ThreadLocalStorageIt() noexcept = default;
  constexpr ThreadLocalStorageIt(ThreadLocalStorageIt&&) noexcept = default;
  constexpr ThreadLocalStorageIt(const ThreadLocalStorageIt&) noexcept = default;
  ~ThreadLocalStorageIt() = default;

  constexpr ThreadLocalStorageIt& operator=(const ThreadLocalStorageIt&) noexcept = default;
  constexpr ThreadLocalStorageIt& operator=(ThreadLocalStorageIt&&) noexcept = default;

  reference operator*() const noexcept {
    return m_curr[m_loc];
  }

  reference operator*() noexcept {
    return m_curr[m_loc];
  }

  pointer operator->() {
    return m_curr.get(m_loc);
  }

  ThreadLocalStorageIt& operator++() {
    m_loc++;
    return *this;
  }

  ThreadLocalStorageIt operator++(int) {
    ThreadLocalStorageIt tmp = *this;
    ++(*this);
    return tmp;
  }

  ThreadLocalStorageIt& operator--() {
    m_loc--;
    return *this;
  }

  ThreadLocalStorageIt operator--(int) {
    ThreadLocalStorageIt tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr ThreadLocalStorageIt operator+(std::uint64_t n) const noexcept {
    return ThreadLocalStorageIt(m_curr, m_loc + n);
  }

  constexpr ThreadLocalStorageIt& operator+=(std::uint64_t n) noexcept {
    m_loc += n;
    return *this;
  }

  constexpr ThreadLocalStorageIt operator-(std::uint64_t n) const noexcept {
    return ThreadLocalStorageIt(m_curr, m_loc - n);
  }

  constexpr difference_type operator-(ThreadLocalStorageIt b) const noexcept {
    return m_loc - b.loc;
  }

  friend bool operator==(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return a.m_loc == b.m_loc;
  }

  friend bool operator!=(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return !(a == b);
  }

  friend bool operator<(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return a.m_loc < b.m_loc;
  }

  friend bool operator<=(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return a.m_loc <= b.m_loc;
  }

  friend bool operator>(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return a.m_loc > b.m_loc;
  }

  friend bool operator>=(const ThreadLocalStorageIt& a, const ThreadLocalStorageIt& b) {
    return a.m_loc >= b.m_loc;
  }

  friend pando::Place localityOf(ThreadLocalStorageIt& a) {
    auto [place, thread] = a.m_curr.getPlaceFromThreadIdx(a.m_loc);
    return place;
  }
};

template <typename T>
[[nodiscard]] pando::Expected<galois::ThreadLocalStorage<T>> copyToAllThreads(T& cont) {
  galois::ThreadLocalStorage<T> ret{};
  PANDO_CHECK_RETURN(ret.initialize());
  PANDO_CHECK_RETURN(galois::doAll(
      cont, ret, +[](T cont, pando::GlobalRef<T> refcopy) {
        T copy;
        const std::uint64_t size = cont.size();
        PANDO_CHECK(copy.initialize(size));
        for (std::uint64_t i = 0; i < cont.size(); i++) {
          copy.get(i) = cont.get(i);
        }
        refcopy = copy;
      }));
  return ret;
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_STORAGE_HPP_
