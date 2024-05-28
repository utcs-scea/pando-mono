// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_CONTAINERS_GVECTOR_HPP_
#define PANDO_RT_CONTAINERS_GVECTOR_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "../memory/allocate_memory.hpp"
#include "../memory/global_ptr.hpp"
#include "../utility/math.hpp"
#include "array.hpp"

namespace pando {

/**
 * @brief Sequence container that stores elements of type @p T contiguously and can change size
 *        dynamically.
 *
 * @note A @c Gvector object is empty upon construction. One of the `Gvector::initialize()` functions
 *       needs to be called to allocate space.
 */
template <typename T>
class Gvector {
  /// @brief The length of the vector
  std::uint64_t m_size = 0;
  /// @brief An array that holds the data
  GlobalPtr<pando::Vector<T>> vec_ptr{nullptr};

public:
  using iterator = GlobalPtr<T>;
  using const_iterator = GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Gvector() noexcept = default;

  // TODO(ypapadop-amd) These are not the intented copy/move constructors/assignment operators.
  // However, due to deficiencies in the GlobalRef, the pattern
  //    Gvector<T> copy = *globalPtr;
  //    mutate(copy);
  //    *globalPtr = copy;
  // is used, which requires Gvector<T> to be a trivially copyable object.

  constexpr Gvector(Gvector&&) noexcept = default;
  constexpr Gvector(const Gvector&) noexcept = default;

  ~Gvector() = default;

  constexpr Gvector& operator=(const Gvector&) noexcept = default;
  constexpr Gvector& operator=(Gvector&&) noexcept = default;

  /**
   * @copydoc initialize(std::uint64_t)
   *
   * @param[in] place      place to allocate memory from
   * @param[in] memoryType memory to allocate from
   */
  [[nodiscard]] Status initialize(std::uint64_t size, Place place, MemoryType memoryType) {
    vec_ptr = PANDO_EXPECT_RETURN(allocateMemory<pando::Vector<T>>(1, place, memoryType));
    return vec_ptr->initialize(size,place,memoryType);
  }

  /**
   * @brief Initializes the vector with the given size in @ref MemoryType::Main memory.
   *
   * @note This function will default initialize all elements.
   *
   * @param[in] size size of vector in elements
   */
  [[nodiscard]] Status initialize(std::uint64_t size) {
    vec_ptr = PANDO_EXPECT_RETURN(allocateMemory<pando::Vector<T>>(1, getCurrentPlace(), MemoryType::Main));
    return vec_ptr->initialize(size,getCurrentPlace(),MemoryType::Main);
  }

  /**
   * @brief Deinitializes the container.
   */
  void deinitialize() {
    lift(vec_ptr, deinitialize);
  }

  /**
   * @brief Returns the memory this vector is allocated in.
   */
  MemoryType getMemoryType() const noexcept {
    return lift(vec_ptr, getMemoryType);
  }

  /**
   * @brief Reserves space in the container for at least @p nextCapacity number of elements.
   *
   * @note If the new capacity is less that the current capacity, the function has not effect.
   * @note The function does not change the size of the container and does not initialize the new
   *       elements.
   * @note If the container has not been initialized, then the memory will be @ref MemoryType::Main.
   *
   * @param[in] nextCapacity new capacity of the container in elements
   */
  [[nodiscard]] Status reserve(std::uint64_t nextCapacity) {
    return fmap(vec_ptr, reserve, nextCapacity);
  }

  /**
   * @brief Reserves the next power of 2 higher than the current size.
   */
  [[nodiscard]] Status growPow2() {
    return lift(vec_ptr, growPow2);
  }

  /**
   * @brief clear the vector
   */
  void clear() {
    return lift(vec_ptr, clear);
  }

  constexpr std::uint64_t capacity() const noexcept {
    pando::GlobalPtr<void> vvec = static_cast<pando::GlobalPtr<void>>(vec_ptr);  
    pando::GlobalPtr<std::byte> bvec = static_cast<pando::GlobalPtr<std::byte>>(vvec); 
    auto offsetVec = bvec + offsetof(pando::Vector<T>,m_size); 
    auto offsetVVec = static_cast<pando::GlobalPtr<void>>(offsetVec); 
    std::uint64_t desired = *static_cast<pando::GlobalPtr<std::uint64_t>> (offsetVVec); 
    return desired; 

  }

  constexpr bool empty() const noexcept {
    return capacity() == 0;
  }

  constexpr auto get(std::uint64_t pos) {
    return vec_ptr->get(pos);
  }

  constexpr auto operator[](std::uint64_t pos) {
    return vec_ptr->get(pos);
  }

  constexpr auto operator[](std::uint64_t pos) const {
    return vec_ptr->get(pos);
  }

  constexpr GlobalPtr<T> data() noexcept {
    return vec_ptr->data();
  }

  constexpr GlobalPtr<const T> data() const noexcept {
    return vec_ptr->data();
  }

  constexpr std::uint64_t size() const noexcept {
    pando::GlobalPtr<void> vvec = static_cast<pando::GlobalPtr<void>>(vec_ptr);  
    pando::GlobalPtr<std::byte> bvec = static_cast<pando::GlobalPtr<std::byte>>(vvec); 
    auto offsetVec = bvec + offsetof(pando::Vector<T>,m_size); 
    auto offsetVVec = static_cast<pando::GlobalPtr<void>>(offsetVec); 
    std::uint64_t desired = *static_cast<pando::GlobalPtr<std::uint64_t>> (offsetVVec); 
    return desired;
  }

  /**
   * @brief Appends the element to the end of the vector.
   *
   * @warning If the operation will increase the size of the container past its capacity, a
   *          realocation takes places.
   *
   * @param[in] value element to append
   */
  [[nodiscard]] Status pushBack(const T& value) {
    return vec_ptr->pushBack(value);
  }

  /**
   * @copydoc pushBack(const T&)
   */
  [[nodiscard]] Status pushBack(T&& value) {
    return fmap(vec_ptr, pushBack, std::move(value));
  }

  // TODO(AdityaAtulTewari) Whenever it is time for performance counters they need to be encoded
  // properly
  /**
   * @brief Copies data from one vector to another
   *
   * @note Super useful for doing bulk data transfers from remote sources
   * @warning Assumes that this vector is not initialized.
   * @warning Will allocate memory in local main memory
   *
   * @param from this is the vector we are copying from
   */
  [[nodiscard]] Status assign(GlobalPtr<Gvector<T>> from) {
    return fmap(vec_ptr, assign, from);
  }

  // TODO(AdityaAtulTewari) Whenever it is time for performance counters they need to be encoded
  // properly
  /**
   * @brief Copies data from one vector and appends it to another
   *
   * @note Super useful for doing bulk data transfers from remote sources
   *
   * @param from this is the vector we are copying from
   */
  [[nodiscard]] Status append(GlobalPtr<Gvector<T>> from) {
    return fmap(vec_ptr, append, from);
  }

  iterator begin() noexcept {
    return iterator(data());
  }

  iterator begin() const noexcept {
    return iterator(data());
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(data());
  }

  iterator end() noexcept {
    return iterator(data() + size());
  }

  iterator end() const noexcept {
    return iterator(data() + size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(data() + size());
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
bool operator==(const Gvector<T>& lhs, const Gvector<T>& rhs) noexcept {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/// @ingroup ROOT
template <typename T>
bool operator!=(const Gvector<T>& lhs, const Gvector<T>& rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace pando

#endif // PANDO_RT_CONTAINERS_VECTOR_HPP_
