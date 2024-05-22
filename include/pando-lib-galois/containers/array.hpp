// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_LIB_GALOIS_CONTAINERS_ARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_ARRAY_HPP_

#include <cstdint>

#include "pando-rt/export.h"

#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/utility/math.hpp>

namespace galois {

/**
 * @brief A dynamic size array that implements the interface for pfxsum
 *
 * @c Array is a container that encapsulates a dynamic size array that is allocated once and does
 * not change size after. The elements are stored contiguously in memory.
 *
 * @warning The elements in the array are not initialized.
 *
 * @note An @c Array object is empty upon construction. One of the `Array::initialize()` functions
 *       needs to be called to allocate space.
 *
 * @tparam T Element type
 */
template <typename T>
class Array {
  pando::GlobalPtr<T> m_data{nullptr};
  std::uint64_t m_size{0};

public:
  using iterator = pando::GlobalPtr<T>;
  using const_iterator = pando::GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Array() noexcept = default;

  constexpr Array(Array&&) noexcept = default;
  constexpr Array(const Array&) noexcept = default;

  ~Array() = default;

  constexpr Array& operator=(const Array&) noexcept = default;
  constexpr Array& operator=(Array&&) noexcept = default;

  /**
   * @copydoc initialize(std::uint64_t)
   *
   * @param[in] place      place to allocate memory from
   * @param[in] memoryType memory to allocate from
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size, pando::Place place,
                                         pando::MemoryType memoryType) {
    using pando::Status, pando::allocateMemory;
    if (size == 0) {
      m_data = nullptr;
      m_size = 0;
      return Status::Success;
    }

    auto result = allocateMemory<T>(size, place, memoryType);
    if (!result.hasValue()) {
      m_data = nullptr;
      m_size = 0;
      return result.error();
    }

    m_data = result.value();
    m_size = size;
    return Status::Success;
  }

  /**
   * @brief Initializes this array by allocating memory for @p size elements in
   *        @ref MemoryType::Main memory.
   *
   * @warning The elements are not initialized, they are left in an indeterminate state.
   *
   * @param[in] size size of the array in elements
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size) {
    const auto place = pando::getCurrentPlace();
    return initialize(size, place, pando::MemoryType::Main);
  }

  /**
   * @brief Deinitializes the array.
   */
  void deinitialize() {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Array only supports trivially destructible types");

    pando::deallocateMemory(m_data, m_size);
    m_data = nullptr;
    m_size = 0;
  }

  constexpr pando::GlobalPtr<T> get(std::uint64_t pos) noexcept {
    return &m_data[pos];
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return m_data[pos];
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return m_data[pos];
  }

  constexpr pando::GlobalPtr<T> data() noexcept {
    return m_data;
  }

  constexpr pando::GlobalPtr<const T> data() const noexcept {
    return m_data;
  }

  constexpr bool empty() const noexcept {
    return m_size == 0;
  }

  constexpr std::uint64_t size() const noexcept {
    return *((std::uint64_t*)(reinterpret_cast<char*>(this) + offsetof(Array<T>, m_size)));
  }

  /**
   * @brief Assigns @p value to all elements in the container.
   */
  constexpr void fill(const T& value) {
    for (std::uint64_t i = 0; i < size(); ++i) {
      m_data[i] = value;
    }
  }

  iterator begin() noexcept {
    return iterator(m_data);
  }

  iterator begin() const noexcept {
    return iterator(m_data);
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(m_data);
  }

  iterator end() noexcept {
    return iterator(m_data + size());
  }

  iterator end() const noexcept {
    return iterator(m_data + size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(m_data + size());
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

template <typename T>
bool operator==(const Array<T>& a, const Array<T>& b) noexcept {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <typename T>
bool operator!=(const Array<T>& a, const Array<T>& b) noexcept {
  return !(a == b);
}

/**
 * @brief Check if @p a and @p b use the same underlying storage.
 *
 */
template <typename T>
bool isSame(const Array<T>& a, const Array<T>& b) noexcept {
  return a.data() == b.data();
}

/**
 * @brief View over a contiguous range of data.
 *
 * TODO(ypapadop-amd): This needs to extend to the rest of the interface of std::span.
 */
template <typename T>
class Span {
  pando::GlobalPtr<T> m_data{};
  std::uint64_t m_size{};

public:
  using iterator = pando::GlobalPtr<T>;
  using const_iterator = pando::GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Span() noexcept = default;

  constexpr Span(pando::GlobalPtr<T> data, std::uint64_t size) noexcept
      : m_data(data), m_size(size) {}

  constexpr pando::GlobalPtr<T> data() const noexcept {
    return m_data;
  }

  constexpr std::uint64_t size() const noexcept {
    return *((std::uint64_t*)(reinterpret_cast<char*>(this) + offsetof(Array<T>, m_size)));
  }

  constexpr pando::GlobalPtr<T> get(std::uint64_t pos) noexcept {
    return &m_data[pos];
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return m_data[pos];
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return m_data[pos];
  }

  iterator begin() noexcept {
    return iterator(m_data);
  }

  iterator begin() const noexcept {
    return iterator(m_data);
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(m_data);
  }

  iterator end() noexcept {
    return iterator(m_data + size());
  }

  iterator end() const noexcept {
    return iterator(m_data + size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(m_data + size());
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

template <typename T>
bool operator==(const Span<T>& a, const Span<T>& b) noexcept {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <typename T>
bool operator!=(const Span<T>& a, const Span<T>& b) noexcept {
  return !(a == b);
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_ARRAY_HPP_
