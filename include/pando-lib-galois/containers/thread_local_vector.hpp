// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_VECTOR_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_THREAD_LOCAL_VECTOR_HPP_
#include <cassert>
#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/array.hpp>
#include <pando-lib-galois/containers/host_cached_array.hpp>
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

  using iterator = ThreadLocalStorageIt<pando::Vector<T>>;
  using reverse_iterator = std::reverse_iterator<iterator>;

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
  std::uint64_t sizeAll() const {
    if (indicesComputed) {
      return *m_indices.rbegin();
    }
    std::uint64_t size = 0;
    for (std::uint64_t i = 0; i < m_data.size(); i++) {
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
                      galois::HostIndexedMap>
        prefixSum(m_data, m_indices);
    PANDO_CHECK_RETURN(prefixSum.initialize(pando::getPlaceDims().node.id));

    prefixSum.computePrefixSumPasteLocality(m_indices.size());
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
  [[nodiscard]] static pando::Expected<std::uint64_t> hostIndexOffset(
      galois::ThreadLocalStorage<std::uint64_t> indices, uint64_t host) noexcept {
    if (host == 0)
      return static_cast<std::uint64_t>(0);
    const auto place =
        pando::Place(pando::NodeIndex(host), pando::PodIndex(0, 0), pando::CoreIndex(0, 0));
    const auto idx = getThreadIdxFromPlace(place, pando::ThreadIndex(0));
    return indices[idx - 1];
  }

  [[nodiscard]] pando::Status hostFlattenAppend(galois::HostLocalStorage<pando::Vector<T>> flat) {
    pando::Status err;

    if (!indicesComputed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::uint64_t i = 0; i < flat.getNumHosts(); i++) {
      auto ref = flat[i];
      std::uint64_t start = PANDO_EXPECT_RETURN(hostIndexOffset(m_indices, i));
      std::uint64_t end = PANDO_EXPECT_RETURN(hostIndexOffset(m_indices, i + 1));
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
      std::uint64_t host = i / getThreadsPerHost();
      std::uint64_t start = PANDO_EXPECT_CHECK(data.hostIndexOffset(data.m_indices, host));
      std::uint64_t curr = (i == 0) ? 0 : data.m_indices[i - 1];
      std::uint64_t end = PANDO_EXPECT_CHECK(data.hostIndexOffset(data.m_indices, host + 1));

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

private:
  class SizeIt {
  public:
    SizeIt() noexcept = default;
    SizeIt(const SizeIt&) = default;
    SizeIt(SizeIt&&) = default;
    ~SizeIt() = default;
    SizeIt& operator=(const SizeIt&) = default;
    SizeIt& operator=(SizeIt&&) = default;
    SizeIt(ThreadLocalStorage<std::uint64_t> indices, std::uint64_t host)
        : m_indices(indices), m_host(host) {}
    using output_type = std::int64_t;
    using difference_type = std::int64_t;

    output_type operator*() const noexcept {
      const std::uint64_t start = PANDO_EXPECT_CHECK(hostIndexOffset(m_indices, m_host));
      const std::uint64_t end = PANDO_EXPECT_CHECK(hostIndexOffset(m_indices, m_host + 1));
      return end - start;
    }

    SizeIt& operator++() {
      m_host++;
      return *this;
    }

    SizeIt operator++(int) {
      SizeIt tmp = *this;
      ++(*this);
      return tmp;
    }

    SizeIt& operator--() {
      m_host--;
      return *this;
    }

    SizeIt operator--(int) {
      SizeIt tmp = *this;
      --(*this);
      return tmp;
    }

    constexpr SizeIt operator+(std::uint64_t n) const noexcept {
      return SizeIt(m_indices, m_host + n);
    }

    constexpr SizeIt& operator+=(std::uint64_t n) noexcept {
      m_host += n;
      return *this;
    }

    constexpr SizeIt operator-(std::uint64_t n) const noexcept {
      return SizeIt(m_indices, m_host - n);
    }

    constexpr difference_type operator-(SizeIt b) const noexcept {
      return m_host - b.host;
    }

    friend bool operator==(const SizeIt& a, const SizeIt& b) {
      return a.m_host == b.m_host && a.m_indices == b.m_indices;
    }

    friend bool operator!=(const SizeIt& a, const SizeIt& b) {
      return !(a == b);
    }

    friend bool operator<(const SizeIt& a, const SizeIt& b) {
      return a.m_host < b.m_host;
    }

    friend bool operator<=(const SizeIt& a, const SizeIt& b) {
      return a.m_host <= b.m_host;
    }

    friend bool operator>(const SizeIt& a, const SizeIt& b) {
      return a.m_host > b.m_host;
    }

    friend bool operator>=(const SizeIt& a, const SizeIt& b) {
      return a.m_host >= b.m_host;
    }

    friend pando::Place localityOf(SizeIt& a) {
      return pando::Place{pando::NodeIndex(a.m_host), pando::anyPod, pando::anyCore};
    }

  private:
    galois::ThreadLocalStorage<std::uint64_t> m_indices;
    std::uint64_t m_host;
  };

  struct SizeRange {
    using iterator = SizeIt;
    galois::ThreadLocalStorage<std::uint64_t> m_indices;
    SizeRange() noexcept = default;
    SizeRange(const SizeRange&) = default;
    SizeRange(SizeRange&&) = default;
    ~SizeRange() = default;
    SizeRange& operator=(const SizeRange&) = default;
    SizeRange& operator=(SizeRange&&) = default;
    explicit SizeRange(ThreadLocalStorage<std::uint64_t> indices) : m_indices(indices) {}
    iterator begin() const noexcept {
      return iterator(m_indices, 0);
    }

    iterator end() const noexcept {
      std::uint64_t numHosts = pando::getPlaceDims().node.id;
      return iterator(m_indices, numHosts);
    }
    std::uint64_t size() const noexcept {
      return pando::getPlaceDims().node.id;
    }
  };

public:
  [[nodiscard]] pando::Expected<galois::HostCachedArray<T>> hostCachedFlatten() {
    if (!indicesComputed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    galois::HostCachedArray<T> hla;
    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    PANDO_CHECK_RETURN(hla.initialize(SizeRange(m_indices)));
    auto tpl = galois::make_tpl(static_cast<ThreadLocalVector>(*this), hla);
    // Reduce into the per host vectors
    auto f = +[](decltype(tpl) assign, std::uint64_t i, uint64_t) {
      auto [data, flat] = assign;
      std::uint64_t host = i / galois::getThreadsPerHost();
      std::uint64_t start = PANDO_EXPECT_CHECK(hostIndexOffset(data.m_indices, host));
      std::uint64_t curr = (i == 0) ? 0 : data.m_indices[i - 1];
      pando::Vector<std::uint64_t> localVec = data[i];
      for (T elt : localVec) {
        flat.getSpecificRef(host, curr - start) = elt;
        curr++;
      }
    };
    galois::onEach(tpl, f);
    return hla;
  }

  iterator begin() noexcept {
    return iterator(this->m_data, 0);
  }

  iterator begin() const noexcept {
    return iterator(this->m_data, 0);
  }

  iterator end() noexcept {
    return iterator(this->m_data, size());
  }

  iterator end() const noexcept {
    return iterator(this->m_data, size());
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
