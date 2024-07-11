// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_CONTAINERS_ARRAY_HPP_
#define PANDO_RT_CONTAINERS_ARRAY_HPP_

#include <cstdint>

#include "../memory/allocate_memory.hpp"
#include "../memory/global_ptr.hpp"
#include "../utility/math.hpp"

namespace pando {

/**
 * @brief A dynamic size array.
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
public:  

  GlobalPtr<T> m_data{nullptr};
  std::uint64_t m_size{0};
  MemoryType m_memoryType{MemoryType::Unknown};

  using iterator = GlobalPtr<T>;
  using const_iterator = GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Array() noexcept = default;

  // TODO(ypapadop-amd) These are not the intented copy/move constructors/assignment operators.
  // However, due to deficiencies in the GlobalRef, the pattern
  //    Array<T> copy = *globalPtr;
  //    mutate(copy);
  //    *globalPtr = copy;
  // is used, which requires Array<T> to be a trivially copyable object.

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
  [[nodiscard]] Status initialize(std::uint64_t size, Place place, MemoryType memoryType) {
    if (size == 0) {
      m_data = nullptr;
      m_size = 0;
      m_memoryType = memoryType;
      return Status::Success;
    }

    auto result = allocateMemory<T>(size, place, memoryType);
    if (!result.hasValue()) {
      m_data = nullptr;
      m_size = 0;
      m_memoryType = MemoryType::Unknown;
      return result.error();
    }

    m_data = result.value();
    m_memoryType = memoryType;
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
  [[nodiscard]] Status initialize(std::uint64_t size) {
    const auto place = getCurrentPlace();
    return initialize(size, place, MemoryType::Main);
  }

  /**
   * @brief Deinitializes the array.
   */
  void deinitialize() {
    // TODO(ypapadop-amd) Only trivially destructible objects are supported, since deinitialize()
    // does not call their destructor.
    static_assert(std::is_trivially_destructible_v<T>,
                  "Array only supports trivially destructible types");

    deallocateMemory(m_data, m_size);
    m_data = nullptr;
    m_size = 0;
    m_memoryType = MemoryType::Unknown;
  }

  MemoryType getMemoryType() const noexcept {
    return m_memoryType;
  }

  constexpr GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return m_data[pos];
  }

  constexpr GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return m_data[pos];
  }

  constexpr GlobalRef<T> get(std::uint64_t pos) noexcept {
    return m_data[pos];
  }

  constexpr GlobalPtr<T> data() noexcept {
    return m_data;
  }

  constexpr GlobalPtr<const T> data() const noexcept {
    return m_data;
  }

  constexpr bool empty() const noexcept {
    return m_size == 0;
  }

  constexpr std::uint64_t size() const noexcept {
    return m_size;
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

/// @ingroup ROOT
template <typename T>
bool operator==(const Array<T>& a, const Array<T>& b) noexcept {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

/// @ingroup ROOT
template <typename T>
bool operator!=(const Array<T>& a, const Array<T>& b) noexcept {
  return !(a == b);
}

/**
 * @brief Check if @p a and @p b use the same underlying storage.
 *
 * @ingroup ROOT
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
  GlobalPtr<T> m_data{};
  std::uint64_t m_size{};

public:
  using iterator = GlobalPtr<T>;
  using const_iterator = GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Span() noexcept = default;

  constexpr Span(GlobalPtr<T> data, std::uint64_t size) noexcept : m_data(data), m_size(size) {}

  constexpr GlobalPtr<T> data() const noexcept {
    return m_data;
  }

  constexpr std::uint64_t size() const noexcept {
    return m_size;
  }

  constexpr GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return m_data[pos];
  }

  constexpr GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
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

/// @ingroup ROOT
template <typename T>
bool operator==(const Span<T>& a, const Span<T>& b) noexcept {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

/// @ingroup ROOT
template <typename T>
bool operator!=(const Span<T>& a, const Span<T>& b) noexcept {
  return !(a == b);
}

} // namespace pando

#endif // PANDO_RT_CONTAINERS_ARRAY_HPP_
