/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/memory/memory_utilities.hpp>

namespace galois {

/**
 * @brief a place and a memoryType for use in constructing a DistArray
 */
struct PlaceType {
  pando::Place place;
  pando::MemoryType memType;
};

/**
 * @brief This is an array like container that spans multiple hosts
 */
template <typename T>
class DistArray {
  /// @brief The data structure storing the data
  pando::Array<pando::Array<T>> m_data;

  /**
   * @brief Returns a pointer to the given index
   */
  pando::GlobalPtr<T> get(std::uint64_t i) {
    if (m_data.size() == 0 || i >= m_data.size() * static_cast<pando::Array<T>>(m_data[0]).size()) {
      return nullptr;
    }
    pando::Array<T> arr = m_data[0];
    const std::uint64_t index = i % arr.size();
    const std::uint64_t blockId = i / arr.size();
    pando::Array<T> blockPtr = m_data[blockId];

    return &(blockPtr[index]);
  }

  /**
   * @brief an iterator that stores the DistArray and the current position to provide random access
   * iterator semantics
   */
  template <typename Y>
  struct DAIterator {
  private:
    DistArray<Y>& m_arr;
    std::uint64_t m_pos;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::int64_t;
    using value_type = T;
    using pointer = pando::GlobalPtr<T>;
    using reference = pando::GlobalRef<T>;

    DAIterator(DistArray<Y>& arr, std::uint64_t pos) : m_arr(arr), m_pos(pos) {}

    reference operator*() const {
      return *m_arr.get(m_pos);
    }

    pointer operator->() {
      return m_arr.get(m_pos);
    }
    DAIterator& operator++() {
      m_pos++;
      return *this;
    }
    DAIterator operator++(int) {
      DAIterator tmp = *this;
      ++(*this);
      return tmp;
    }
    friend bool operator==(const DAIterator<Y>& a, const DAIterator<Y>& b) {
      return &a.m_arr == &b.m_arr && a.m_pos == b.m_pos;
    }
    friend bool operator!=(const DAIterator<Y>& a, const DAIterator<Y>& b) {
      return &a.m_arr != &b.m_arr || a.m_pos != b.m_pos;
    }
    /**
    template <>
    friend pando::Place pando::localityOf<DAIterator<Y>>(const DAIterator<Y>& a) {
      return pando::localityOf(a.m_arr.get(m_pos));
    }
    */
  };

public:
  using iterator = DAIterator<T>;
  using const_iterator = DAIterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  DistArray() noexcept = default;

  DistArray(const DistArray&) = default;
  DistArray(DistArray&&) = default;

  ~DistArray() = default;

  DistArray& operator=(const DistArray&) = default;
  DistArray& operator=(DistArray&&) = default;

  /**
   * @brief Takes in iterators with semantics like memoryType and a size to initialize the sizes of
   * the objects
   *
   * @tparam It the iterator type
   * @param[in] beg The beginning of the iterator to memoryType like objects
   * @param[in] end The end of the iterator to memoryType like objects
   * @param[in] size The size of the data to encapsulate in this abstraction
   */
  template <typename It>
  [[nodiscard]] pando::Status initialize(It beg, It end, std::uint64_t size) {
    if (m_data.data() != nullptr) {
      return pando::Status::AlreadyInit;
    }

    if (size == 0) {
      return pando::Status::Success;
    }

    const std::uint64_t buckets = std::distance(beg, end);

    const std::uint64_t bucketSize = (size / buckets) + (size % buckets ? 1 : 0);

    pando::Status err = m_data.initialize(buckets);
    if (err != pando::Status::Success) {
      return err;
    }

    for (std::uint64_t i = 0; beg != end; i++, beg++) {
      pando::Array<T> arr;
      typename std::iterator_traits<It>::value_type val = *beg;
      err = arr.initialize(bucketSize, val.place, val.memType);
      if (err != pando::Status::Success) {
        return err;
      }
      m_data[i] = arr;
    }
    return err;
  }

  /**
   * @brief Deinitializes the array.
   */
  void deinitialize() {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Array only supports trivially destructible types");

    if (m_data.data() == nullptr) {
      return;
    }
    for (auto array : m_data) {
      static_cast<pando::Array<std::uint64_t>>(array).deinitialize();
    }
    m_data.deinitialize();
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return *get(pos);
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return *get(pos);
  }

  constexpr bool empty() const noexcept {
    return m_data.size() == 0;
  }

  std::uint64_t size() noexcept {
    if (m_data.size() == 0)
      return 0;
    pando::Array<T> arr = m_data[0];
    return m_data.size() * arr.size();
  }

  iterator begin() noexcept {
    return iterator(*this, 0);
  }

  iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(*this, 0);
  }

  iterator end() noexcept {
    return iterator(*this, size());
  }

  iterator end() const noexcept {
    return iterator(*this, size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(*this, size());
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
} // namespace galois
#endif // PANDO_LIB_GALOIS_CONTAINERS_DIST_ARRAY_HPP_
