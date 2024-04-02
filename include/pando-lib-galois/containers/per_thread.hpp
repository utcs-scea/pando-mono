// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_PER_THREAD_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
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

  uint64_t getLocalVectorID() {
    uint64_t coreID = (pando::getCurrentPlace().core.x * coreY) + pando::getCurrentPlace().core.y;
    return ((pando::getCurrentPlace().node.id * cores * threads) + (coreID * threads) +
            pando::getCurrentThread().id);
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

    PANDO_CHECK_RETURN(m_data.initialize(hosts * cores * threads));

    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      pando::Vector<T> vec;
      PANDO_CHECK_RETURN(
          vec.initialize(0, pando::localityOf(m_data.get(i)), pando::MemoryType::Main));
      m_data[i] = vec;
    }
    return pando::Status::Success;
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
    PANDO_CHECK_RETURN(err);
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

  void clear() {
    indices_computed = false;
    for (std::uint64_t i = 0; i < m_data.size(); i++) {
      liftVoid(m_data[i], clear);
    }
  }

  /**
   * @brief Returns the global index that elements for host start
   *
   * @param host passing in `hosts + 1` is legal
   * @param index passed by reference will hold the global index
   */
  [[nodiscard]] pando::Status hostIndexOffset(uint64_t host, uint64_t& index) {
    if (!indices_computed) {
      PANDO_CHECK_RETURN(computeIndices());
    }
    index = (host == 0) ? 0 : m_indices[host * cores * threads - 1];
    return pando::Status::Success;
  }

  /**
   * @brief Returns the global index that elements for the local host start
   *
   * @param index passed by reference will hold the global index
   */
  [[nodiscard]] pando::Status currentHostIndexOffset(uint64_t& elts) {
    return hostIndexOffset(pando::getCurrentNode().id, elts);
  }

  /**
   * @brief Returns the total number of elements on a specific host
   *
   * @param host
   * @param elts passed by reference will hold the number of elements
   */
  [[nodiscard]] pando::Status elementsOnHost(uint64_t host, uint64_t& elts) {
    std::uint64_t start;
    PANDO_CHECK_RETURN(hostIndexOffset(host, start));
    std::uint64_t end;
    PANDO_CHECK_RETURN(hostIndexOffset(host + 1, end));
    elts = end - start;
    return pando::Status::Success;
  }

  /**
   * @brief Returns the total number of elements on the local host
   *
   * @param elts passed by reference will hold the number of elements
   */
  [[nodiscard]] pando::Status localElements(uint64_t& elts) {
    return elementsOnHost(pando::getCurrentNode().id, elts);
  }

  /**
   * @brief Returns the global start index of the local thread vector
   *
   * @param thread
   * @param index passed by reference will hold the start index for thread
   */
  [[nodiscard]] pando::Status indexOnThread(uint64_t thread, uint64_t& index) {
    if (!indices_computed) {
      PANDO_CHECK_RETURN(computeIndices());
    }
    index = (thread == 0) ? 0 : m_indices[thread - 1];
    return pando::Status::Success;
  }

  /**
   * @brief Returns the global start index of the current local thread vector
   *
   * @param index passed by reference will hold the start index for the current thread
   */
  [[nodiscard]] pando::Status getLocalIndex(uint64_t& index) {
    return indexOnThread(getLocalVectorID(), index);
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
    if (!indices_computed) {
      PANDO_CHECK_RETURN(computeIndices());
    }
    PANDO_CHECK_RETURN(to.initialize(sizeAll()));
    galois::onEach(
        AssignState<galois::DistArray>(*this, to),
        +[](AssignState<galois::DistArray>& state, uint64_t i, uint64_t) {
          uint64_t pos = i == 0 ? 0 : state.data.m_indices[i - 1];
          pando::Vector<T> localVec = state.data[i];
          for (T elt : localVec) {
            state.to[pos++] = elt;
          }
        });
    return pando::Status::Success;
  }

  template <typename Y>
  using HLSV = galois::HostLocalStorage<pando::Vector<Y>>;

  [[nodiscard]] pando::Status hostFlatten(
      pando::GlobalRef<galois::HostLocalStorage<pando::Vector<T>>> flat) {
    pando::Status err;
    err = lift(flat, initialize);
    PANDO_CHECK_RETURN(err);

    if (!indices_computed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::int16_t i = 0; i < static_cast<std::int16_t>(lift(flat, getNumHosts)); i++) {
      auto place = pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore};
      auto ref = fmap(flat, operator[], i);
      std::uint64_t start =
          (i == 0) ? 0 : m_indices[static_cast<std::uint64_t>(i) * cores * threads - 1];
      std::uint64_t end = m_indices[static_cast<std::uint64_t>(i + 1) * cores * threads - 1];
      err = fmap(ref, initialize, end - start, place, pando::MemoryType::Main);
      PANDO_CHECK_RETURN(err);
    }

    // Reduce into the per host vectors
    auto f = +[](AssignState<HLSV> assign, std::uint64_t i, uint64_t) {
      std::uint64_t host = i / (assign.data.cores * assign.data.threads);
      std::uint64_t index =
          static_cast<std::uint64_t>(host) * assign.data.cores * assign.data.threads - 1;
      std::uint64_t start = (host == 0) ? 0 : assign.data.m_indices[index];
      std::uint64_t curr = (i == 0) ? 0 : assign.data.m_indices[i - 1];

      auto ref = assign.to[host];
      pando::Vector<T> localVec = assign.data[i];
      for (T elt : localVec) {
        fmap(ref, operator[], curr - start) = elt;
        curr++;
      }
    };
    galois::onEach(AssignState<HLSV>(*this, flat), f);
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status hostFlattenAppend(galois::HostLocalStorage<pando::Vector<T>> flat) {
    pando::Status err;

    if (!indices_computed) {
      PANDO_CHECK_RETURN(computeIndices());
    }

    // TODO(AdityaAtulTewari) Make this properly parallel.
    // Initialize the per host vectors
    for (std::int16_t i = 0; i < static_cast<std::int16_t>(flat.getNumHosts()); i++) {
      auto ref = flat[i];
      std::uint64_t start =
          (i == 0) ? 0 : m_indices[static_cast<std::uint64_t>(i) * cores * threads - 1];
      std::uint64_t end = m_indices[static_cast<std::uint64_t>(i + 1) * cores * threads - 1];
      err = fmap(ref, reserve, lift(ref, size) + end - start);
      PANDO_CHECK_RETURN(err);
      for (std::uint64_t j = 0; j < end - start; j++) {
        PANDO_CHECK_RETURN(fmap(ref, pushBack, T()));
      }
    }

    // Reduce into the per host vectors
    auto f = +[](AssignState<HLSV> assign, std::uint64_t i, uint64_t) {
      std::uint64_t host = i / (assign.data.cores * assign.data.threads);
      std::uint64_t index =
          static_cast<std::uint64_t>(host) * assign.data.cores * assign.data.threads - 1;
      std::uint64_t start = (host == 0) ? 0 : assign.data.m_indices[index];
      std::uint64_t curr = (i == 0) ? 0 : assign.data.m_indices[i - 1];
      std::uint64_t end =
          assign.data.m_indices[(host + 1) * assign.data.cores * assign.data.threads - 1];

      auto ref = assign.to[host];
      pando::Vector<T> localVec = assign.data[i];
      std::uint64_t size = lift(ref, size) - (end - start);
      for (T elt : localVec) {
        fmap(ref, get, size + curr - start) = elt;
        curr++;
      }
    };
    galois::onEach(AssignState<HLSV>(*this, flat), f);
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status computeIndices() {
    if (m_indices.m_data.data() == nullptr) {
      pando::Status err = m_indices.initialize(hosts * cores * threads);
      PANDO_CHECK_RETURN(err);
    }

    using SRC = galois::DistArray<pando::Vector<T>>;
    using DST = galois::DistArray<uint64_t>;
    using SRC_Val = pando::Vector<T>;
    using DST_Val = uint64_t;

    galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, transmute, scan_op, combiner, galois::DistArray>
        prefixSum(m_data, m_indices);
    PANDO_CHECK_RETURN(prefixSum.initialize());

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
  using const_reference = pando::GlobalRef<const pando::Vector<T>>;

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

  const_reference operator[](std::uint64_t n) const noexcept {
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
