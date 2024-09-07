// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_LIB_GALOIS_CONTAINERS_GARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_GARRAY_HPP_

#include <cstdint>

#include "pando-rt/export.h"

#include <pando-rt/container/array.hpp>
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
class GArray {
  pando::GlobalPtr<galois::Array<T>> arrary_ptr{nullptr};
  std::uint64_t m_size{};

public:
  using iterator = pando::GlobalPtr<T>;
  using const_iterator = pando::GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr GArray() noexcept = default;

  constexpr GArray(GArray&&) noexcept = default;
  constexpr GArray(const GArray&) noexcept = default;

  ~GArray() = default;

  constexpr GArray& operator=(const GArray&) noexcept = default;
  constexpr GArray& operator=(GArray&&) noexcept = default;

  /**
   * @copydoc initialize(std::uint64_t)
   *
   * @param[in] place      place to allocate memory from
   * @param[in] memoryType memory to allocate from
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size, pando::Place place,
                                         pando::MemoryType memoryType) {
    using pando::Status, pando::allocateMemory;
    auto ret = fmap(arrary_ptr, initialize, size, place, memoryType);
    return ret;
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
    return fmap(arrary_ptr, initialize, size, place, pando::MemoryType::Main);
  }

  /**
   * @brief Deinitializes the array.
   */
  void deinitialize() {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Array only supports trivially destructible types");
    lift(arrary_ptr, deinitialize);
    arrary_ptr = nullptr;
  }

  constexpr pando::GlobalPtr<T> get(std::uint64_t pos) noexcept {
    return arrary_ptr->get(pos);
  }

  constexpr pando::GlobalRef<T> operator[](std::uint64_t pos) noexcept {
    return arrary_ptr[pos];
  }

  constexpr pando::GlobalRef<const T> operator[](std::uint64_t pos) const noexcept {
    return arrary_ptr[pos];
  }

  constexpr pando::GlobalPtr<T> data() noexcept {
    return arrary_ptr->m_data;
  }

  constexpr pando::GlobalPtr<const T> data() const noexcept {
    return arrary_ptr->m_data;
  }

  constexpr bool empty() const noexcept {
    return size() == 0;
    return m_size == 0;
  }

  constexpr std::uint64_t size() const noexcept {
    pando::GlobalPtr<void> vvec = static_cast<pando::GlobalPtr<void>>(arrary_ptr);
    pando::GlobalPtr<std::byte> bvec = static_cast<pando::GlobalPtr<std::byte>>(vvec);
    auto offsetVec = bvec + offsetof(galois::Array, m_size);
    auto offsetVVec = static_cast<pando::GlobalPtr<void>>(offsetVec);
    std::uint64_t desired = *static_cast<pando::GlobalPtr<std::uint64_t>>(offsetVVec);
    return desired;
  }

  /**
   * @brief Assigns @p value to all elements in the container.
   */
  constexpr void fill(const T& value) {
    arrary_ptr->fill(value);
  }

  iterator begin() noexcept {
    return arrary_ptr->begin();
  }

  iterator begin() const noexcept {
    return arrary_ptr->begin();
  }

  const_iterator cbegin() const noexcept {
    return arrary_ptr->cbegin();
  }

  iterator end() noexcept {
    return arrary_ptr->end();
  }

  iterator end() const noexcept {
    return arrary_ptr->end();
  }

  const_iterator cend() const noexcept {
    return arrary_ptr->cend();
  }

  /**
   * @brief reverse iterator to the first element
   */
  reverse_iterator rbegin() noexcept {
    return arrary_ptr->rbegin();
  }

  /**
   * @copydoc rbegin()
   */
  reverse_iterator rbegin() const noexcept {
    return arrary_ptr->rbegin();
  }

  /**
   * @copydoc rbegin()
   */
  const_reverse_iterator crbegin() const noexcept {
    return arrary_ptr->crbegin();
  }

  /**
   * reverse iterator to the last element
   */
  reverse_iterator rend() noexcept {
    return arrary_ptr->rend();
  }

  /**
   * @copydoc rend()
   */
  reverse_iterator rend() const noexcept {
    return arrary_ptr->rend();
  }

  /**
   * @copydoc rend()
   */
  const_reverse_iterator crend() const noexcept {
    return arrary_ptr->crend();
  }
};

template <typename T>
bool operator==(const GArray<T>& a, const GArray<T>& b) noexcept {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <typename T>
bool operator!=(const GArray<T>& a, const GArray<T>& b) noexcept {
  return !(a == b);
}

/**
 * @brief Check if @p a and @p b use the same underlying storage.
 *
 */
template <typename T>
bool isSame(const GArray<T>& a, const GArray<T>& b) noexcept {
  return a.data() == b.data();
}

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_GARRAY_HPP_
