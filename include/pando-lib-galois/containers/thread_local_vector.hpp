// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_VECTOR_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_VECTOR_HPP_
#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/thread_local_storage.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/prefix_sum.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace galois {

template <typename T>
class ThreadLocalVector {
public:
  ThreadLocalVector() noexcept = default;
  ThreadLocalVector(const ThreadLocalVector&) = default;
  ThreadLocalVector(ThreadLocalVector&&) = default;

  ~ThreadLocalVector() = default;

  ThreadLocalVector& operator=(const ThreadLocalVector&) = default;
  ThreadLocalVector& operator=(ThreadLocalVector&&) = default;

  using iterator = ThreadLocalStorageIt<T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using pointer = pando::GlobalPtr<pando::Vector<T>>;

  [[nodiscard]] pando::Status initialize() {
    pando::Vector<T> vec;
    PANDO_CHECK_RETURN(vec.initialize(0));
    this->m_data = PANDO_EXPECT_RETURN(galois::copyToAllThreads(vec));
    return pando::Status::Success;
  }

  void deinitialize() {
    if (indicesInitialized) {
      m_indices.deinitialize();
    }
    for (pando::Vector<T> vec : m_data) {
      vec.deinitialize();
    }
    m_data.deinitialize();
  }

  pando::GlobalPtr<pando::Vector<T>> getLocal() {
    return m_data.getLocal();
  }

  pando::GlobalPtr<const pando::Vector<T>> getLocal() const noexcept {
    return m_data.getLocal();
  }

  pando::GlobalRef<pando::Vector<T>> getLocalRef() {
    return *m_data.getLocal();
  }

  pando::GlobalRef<const pando::Vector<T>> getLocalRef() const noexcept {
    return *m_data.getLocal();
  }

  pando::GlobalPtr<pando::Vector<T>> get(std::uint64_t i) noexcept {
    return m_data.get(i);
  }

  pando::GlobalPtr<const pando::Vector<T>> get(std::uint64_t i) const noexcept {
    return m_data.get(i);
  }

  constexpr pando::GlobalRef<pando::Vector<T>> operator[](std::uint64_t pos) noexcept {
    return *get(pos);
  }

  constexpr pando::GlobalRef<const pando::Vector<T>> operator[](std::uint64_t pos) const noexcept {
    return *get(pos);
  }

  /**
   * @brief Appends to the current hardware thread's vector.
   */
  [[nodiscard]] pando::Status pushBack(T val) {
    return fmap(this->getLocalRef(), pushBack, val);
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

  void clear() {
    indicesComputed = false;
    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      liftVoid(m_data[i], clear);
    }
  }

private:
  static uint64_t transmute(pando::Vector<T> p) {
    return p.size();
  }
  static uint64_t scan_op(pando::Vector<T> p, uint64_t l) {
    return p.size() + l;
  }
  static uint64_t combiner(uint64_t f, uint64_t s) {
    return f + s;
  }

public:
  [[nodiscard]] pando::Status computeIndices() {
    if (!indicesInitialized) {
      PANDO_CHECK_RETURN(m_indices.initialize());
      indicesInitialized = true;
    }

    using SRC = galois::ThreadLocalStorage<pando::Vector<T>>;
    using DST = galois::ThreadLocalStorage<uint64_t>;
    using SRC_Val = pando::Vector<T>;
    using DST_Val = uint64_t;

    galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, transmute, scan_op, combiner,
                      galois::ThreadLocalStorage>
        prefixSum(m_data, m_indices);
    PANDO_CHECK_RETURN(prefixSum.initialize());

    prefixSum.computePrefixSum(m_indices.size());
    indicesComputed = true;

    prefixSum.deinitialize();
    return pando::Status::Success;
  }

  /**
   * @brief Returns the global index that elements for host start
   *
   * @param host passing in `hosts + 1` is legal
   * @param index passed by reference will hold the global index
   */
  [[nodiscard]] pando::Expected<std::uint64_t> hostIndexOffset(uint64_t host) {
    if (!indicesComputed) {
      return pando::Status::NotInit;
    }
    if (host == 0)
      return 0;
    const auto place =
        pando::Place(pando::NodeIndex(host), pando::PodIndex(0, 0), pando::CoreIndex(0, 0));
    const auto idx = m_data.getThreadIdxFromPlace(place, pando::ThreadIndex(0));
    return m_indices[idx - 1];
  }

  [[nodiscard]] pando::Status hostFlattenAppend(galois::HostLocalStorage<pando::Vector<T>> flat) {
    pando::Status err;

    if (!indicesComputed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::uint64_t i = 0; i < flat.getNumHosts(); i++) {
      auto ref = flat.get(i);
      std::uint64_t start = PANDO_EXPECT_RETURN(hostIndexOffset(i));
      std::uint64_t end = PANDO_EXPECT_RETURN(hostIndexOffset(i + 1));
      err = fmap(ref, reserve, lift(ref, size) + end - start);
      PANDO_CHECK_RETURN(err);
      for (std::uint64_t j = 0; j < end - start; j++) {
        PANDO_CHECK_RETURN(fmap(ref, pushBack, T()));
      }
    }

    auto tpl = galois::make_tpl(static_cast<ThreadLocalVector>(*this), flat);
    // Reduce into the per host vectors
    auto f = +[](decltype(tpl) assign, std::uint64_t i, uint64_t) {
      auto [data, flat] = assign;
      std::uint64_t host = i / ThreadLocalStorage<T>::getThreadsPerHost();
      std::uint64_t hostIndex =
          static_cast<std::uint64_t>(host) * ThreadLocalStorage<T>::getThreadsPerHost() - 1;
      std::uint64_t start = PANDO_EXPECT_RETURN(data.hostIndexOffset(host));
      std::uint64_t curr = (i == 0) ? 0 : data.m_indices[i - 1];
      std::uint64_t end = PANDO_EXPECT_RETURN(data.hostIndexOffset(host + 1));

      auto ref = flat[host];
      pando::Vector<T> localVec = data[i];
      std::uint64_t size = lift(ref, size) - (end - start);
      for (T elt : localVec) {
        fmap(ref, get, size + curr - start) = elt;
        curr++;
      }
    };
    galois::onEach(tpl, f);
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Expected<galois::HostLocalStorage<pando::Array<T>>> assign() {
    galois::HostLocalStorage<pando::Array<T>> hls{};
    PANDO_CHECK_RETURN(hls.initialize());

    if (!indicesComputed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::uint64_t i = 0; i < hls.getNumHosts(); i++) {
      auto ref = hls[i];
      std::uint64_t start = PANDO_EXPECT_RETURN(hostIndexOffset(i));
      std::uint64_t end = PANDO_EXPECT_RETURN(hostIndexOffset(i + 1));
      PANDO_CHECK_RETURN(fmap(ref, initialize, lift(ref, size) + end - start));
    }
    auto tpl = galois::make_tpl(static_cast<ThreadLocalVector>(*this), hls);
    // Reduce into the per host vectors
    auto f = +[](decltype(tpl) assign, std::uint64_t i, uint64_t) {
      auto [data, flat] = assign;
      std::uint64_t host = i / ThreadLocalStorage<T>::getThreadsPerHost();
      std::uint64_t hostIndex =
          static_cast<std::uint64_t>(host) * ThreadLocalStorage<T>::getThreadsPerHost() - 1;
      std::uint64_t start = PANDO_EXPECT_RETURN(data.hostIndexOffset(host));
      std::uint64_t curr = (i == 0) ? 0 : data.m_indices[i - 1];
      std::uint64_t end = PANDO_EXPECT_RETURN(data.hostIndexOffset(host + 1));

      auto ref = flat[host];
      pando::Vector<T> localVec = data[i];
      std::uint64_t size = lift(ref, size) - (end - start);
      for (T elt : localVec) {
        fmap(ref, get, size + curr - start) = elt;
        curr++;
      }
    };
    galois::onEach(tpl, f);
    return hls;
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

private:
  galois::ThreadLocalStorage<pando::Vector<T>> m_data;
  galois::ThreadLocalStorage<std::uint64_t> m_indices;
  bool indicesInitialized = false;
  bool indicesComputed = false;
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_VECTOR_HPP_
