// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/prefix_sum.hpp>
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
  /// @brief Stores a prefix sum of the structure, must be computed manually
  galois::DistArray<uint64_t> m_indices;
  /// @brief Tells if prefix sum has been computed
  bool indices_computed = false;

  uint64_t coreY;
  uint64_t cores;
  uint64_t threads;
  uint64_t hosts;

private:
  uint64_t getLocalVectorID() {
    uint64_t coreID = (pando::getCurrentPlace().core.x * coreY) + pando::getCurrentPlace().core.y;
    return ((pando::getCurrentPlace().node.id * cores * threads) + (coreID * threads) +
            pando::getCurrentThread().id);
  }

  static uint64_t transmute(pando::Vector<T> p) {
    return p.size();
  }
  static uint64_t scan_op(pando::Vector<T> p, uint64_t l) {
    return p.size() + l;
  }
  static uint64_t combiner(uint64_t f, uint64_t s) {
    return f + s;
  }

  template <template <typename> typename Cont>
  struct AssignState {
    AssignState() = default;
    AssignState(PerThreadVector<T> data_, Cont<T> to_) : data(data_), to(to_) {}

    PerThreadVector<T> data;
    Cont<T> to;
  };

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
    indices_computed = false;
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
    if (m_indices.m_data.data() != nullptr) {
      m_indices.deinitialize();
    }
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
   * @warning Assumes that the DistArray "to" is not initialized.
   * @warning Will allocate memory in local main memory
   *
   * @param to this is the DistArray we are copying to
   */
  [[nodiscard]] pando::Status assign(galois::DistArray<T>& to) {
    pando::Status err;
    if (!indices_computed) {
      err = computeIndices();
      if (err != pando::Status::Success) {
        return err;
      }
    }
    err = to.initialize(sizeAll());
    if (err != pando::Status::Success) {
      return err;
    }
    galois::doAll(
        AssignState<galois::DistArray>(*this, to), galois::IotaRange(0, size()),
        +[](AssignState<galois::DistArray>& state, uint64_t i) {
          uint64_t pos = i == 0 ? 0 : state.data.m_indices[i - 1];
          pando::Vector<T> localVec = state.data[i];
          for (T elt : localVec) {
            state.to[pos++] = elt;
          }
        },
        +[](AssignState<galois::DistArray>& state, uint64_t i) {
          return pando::Place{
              pando::NodeIndex{(int16_t)(i / state.data.threads / state.data.cores)}, pando::anyPod,
              pando::anyCore};
        });
    return pando::Status::Success;
  }

  template <typename Y>
  using PHV = galois::PerHost<pando::Vector<Y>>;

  [[nodiscard]] pando::Status hostFlatten(
      pando::GlobalRef<galois::PerHost<pando::Vector<T>>> flat) {
    pando::Status err;
    err = lift(flat, initialize);
    if (err != pando::Status::Success) {
      return err;
    }

    if (!indices_computed) {
      err = computeIndices();
      if (err != pando::Status::Success) {
        return err;
      }
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::int16_t i = 0; i < static_cast<std::int16_t>(lift(flat, getNumNodes)); i++) {
      auto place = pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore};
      auto ref = fmap(flat, get, i);
      err =
          fmap(ref, initialize, m_indices[static_cast<std::uint64_t>(i + 1) * cores * threads - 1],
               place, pando::MemoryType::Main);
      if (err != pando::Status::Success) {
        return err;
      }
    }

    // Reduce into the per host vectors
    auto f = +[](AssignState<PHV> assign, std::uint64_t i) {
      std::uint64_t host = i / (assign.data.cores * assign.data.threads);
      std::uint64_t start = assign.data.m_indices[static_cast<std::uint64_t>(i + 1) *
                                                      assign.data.cores * assign.data.threads -
                                                  1];
      std::uint64_t curr = (i == 0) ? 0 : assign.data.m_indices[i - 1];

      auto ref = assign.to.get(host);
      pando::Vector<T> localVec = assign.data[i];
      for (T elt : localVec) {
        fmap(ref, get, curr - start) = elt;
      }
    };
    galois::doAll(
        AssignState<PHV>(*this, flat), galois::IotaRange(0, size()), f,
        +[](AssignState<PHV>& state, uint64_t i) {
          return pando::Place{
              pando::NodeIndex{(int16_t)(i / state.data.threads / state.data.cores)}, pando::anyPod,
              pando::anyCore};
        });
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status computeIndices() {
    if (m_indices.m_data.data() != nullptr) {
      return pando::Status::AlreadyInit;
    }
    pando::Status err = m_indices.initialize(hosts * cores * threads);
    if (err != pando::Status::Success) {
      return err;
    }
    using SRC = galois::DistArray<pando::Vector<T>>;
    using DST = galois::DistArray<uint64_t>;
    using SRC_Val = pando::Vector<T>;
    using DST_Val = uint64_t;

    galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, transmute, scan_op, combiner, galois::DistArray>
        prefixSum(m_data, m_indices);
    err = prefixSum.initialize();
    if (err != pando::Status::Success) {
      return err;
    }
    prefixSum.computePrefixSum(m_indices.size());
    indices_computed = true;

    prefixSum.deinitialize();
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
