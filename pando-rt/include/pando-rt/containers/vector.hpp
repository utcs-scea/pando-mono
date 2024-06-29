// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_CONTAINERS_VECTOR_HPP_
#define PANDO_RT_CONTAINERS_VECTOR_HPP_

#include <cstdint>
#include <iterator>
#include <utility>

#include "../memory/allocate_memory.hpp"
#include "../memory/global_ptr.hpp"
#include "../utility/math.hpp"
#include "../utility/check.hpp"
#include "array.hpp"

namespace pando {

/**
 * @brief Sequence container that stores elements of type @p T contiguously and can change size
 *        dynamically.
 *
 * @note A @c Vector object is empty upon construction. One of the `Vector::initialize()` functions
 *       needs to be called to allocate space.
 */
template <typename T>
class Vector {
  /// @brief The length of the vector
  std::uint64_t m_size = 0;
  /// @brief An array that holds the data
  Array<T> m_buf;

public:
  using iterator = GlobalPtr<T>;
  using const_iterator = GlobalPtr<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Vector() noexcept = default;

  // TODO(ypapadop-amd) These are not the intented copy/move constructors/assignment operators.
  // However, due to deficiencies in the GlobalRef, the pattern
  //    Vector<T> copy = *globalPtr;
  //    mutate(copy);
  //    *globalPtr = copy;
  // is used, which requires Vector<T> to be a trivially copyable object.

  constexpr Vector(Vector&&) noexcept = default;
  constexpr Vector(const Vector&) noexcept = default;

  ~Vector() = default;

  constexpr Vector& operator=(const Vector&) noexcept = default;
  constexpr Vector& operator=(Vector&&) noexcept = default;

  /**
   * @copydoc initialize(std::uint64_t)
   *
   * @param[in] place      place to allocate memory from
   * @param[in] memoryType memory to allocate from
   */
  [[nodiscard]] Status initialize(std::uint64_t size, Place place, MemoryType memoryType) {
    const auto status = m_buf.initialize(size, place, memoryType);
    if (status == Status::Success) {
      m_size = size;

      // default initialize elements
      m_buf.fill(T{});
    }
    return status;
  }

  /**
   * @brief Initializes the vector with the given size in @ref MemoryType::Main memory.
   *
   * @note This function will default initialize all elements.
   *
   * @param[in] size size of vector in elements
   */
  [[nodiscard]] Status initialize(std::uint64_t size) {
    return initialize(size, getCurrentPlace(), MemoryType::Main);
  }

  /**
   * @brief Deinitializes the container.
   */
  void deinitialize() {
    m_buf.deinitialize();
    m_size = 0;
  }

  /**
   * @brief Returns the memory this vector is allocated in.
   */
  MemoryType getMemoryType() const noexcept {
    return m_buf.getMemoryType();
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
    if (nextCapacity <= capacity()) {
      return Status::Success;
    }

    Array<T> newArray;
    auto status = newArray.initialize(nextCapacity, pando::localityOf(m_buf.data()), getMemoryType());
    if (status != Status::Success) {
      return Status::BadAlloc;
    }

    for (std::uint64_t i = 0; i < size(); i++) {
      newArray[i] = std::move<T>(m_buf[i]);
    }

    std::swap(m_buf, newArray);
    newArray.deinitialize();

    return Status::Success;
  }

  /**
   * @brief this function resizes the array
   *
   * @note the implementation is simple because T must be trivially copyable.
   *
   * @param[in] newSize the new desired size
   **/
  [[nodiscard]] Status resize(std::uint64_t newSize) {
    if(capacity() >= newSize){
      m_size = newSize;
      return Status::Success;
    }
    PANDO_CHECK_RETURN(growPow2(newSize - 1));
    //Subtract one here because we want to be at size or larger
    assert(capacity() >= newSize);
    m_size = newSize;
    return Status::Success;
  }

  /**
   * @brief Reserves the next power of 2 higher than the current size.
   */
  [[nodiscard]] Status growPow2(std::uint64_t biggerThan) {
    if (m_buf.data() == nullptr && biggerThan == 0) {
      return reserve(1);
    }
    if (log2floor(m_buf.size()) >= (sizeof(std::uint64_t) * 8 - 1)) {
      return Status::OutOfBounds;
    }
    const auto nextCapacity = up2(biggerThan);
    return reserve(nextCapacity);
  }

  /**
   * @brief clear the vector
   */
  void clear() {
    m_size = 0;
  }

  constexpr std::uint64_t capacity() const noexcept {
    return m_buf.size();
  }

  constexpr bool empty() const noexcept {
    return m_size == 0;
  }

  constexpr auto get(std::uint64_t pos) {
    return m_buf[pos];
  }

  constexpr auto operator[](std::uint64_t pos) {
    return m_buf[pos];
  }

  constexpr auto operator[](std::uint64_t pos) const {
    return m_buf[pos];
  }

  constexpr GlobalPtr<T> data() noexcept {
    return m_buf.data();
  }

  constexpr GlobalPtr<const T> data() const noexcept {
    return m_buf.data();
  }

  constexpr std::uint64_t size() const noexcept {
    return m_size;
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
    if (m_size == capacity()) {
      auto status = growPow2(m_size);
      if (Status::Success != status) {
        return status;
      }
    }
    m_buf[m_size] = value;
    m_size++;
    return Status::Success;
  }

  /**
   * @copydoc pushBack(const T&)
   */
  [[nodiscard]] Status pushBack(T&& value) {
    if (m_size == capacity()) {
      auto status = growPow2(m_size);
      if (Status::Success != status) {
        return status;
      }
    }
    m_buf[m_size] = std::move(value);
    m_size++;
    return Status::Success;
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
  [[nodiscard]] Status assign(GlobalPtr<Vector<T>> from) {
    Vector<T> tfrom = *from;
    const auto size = tfrom.size();
    auto err = initialize(size);
    if (err != Status::Success) {
      return err;
    }
    detail::bulkMemcpy(tfrom.data().address, sizeof(T) * size, data().address);
    return Status::Success;
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
  [[nodiscard]] Status append(GlobalPtr<Vector<T>> from) {
    Vector<T> tfrom = *from;
    const auto originalSize = size();
    const auto appendSize = tfrom.size();
    PANDO_CHECK_RETURN(resize(originalSize + appendSize));
    detail::bulkMemcpy(tfrom.data().address, sizeof(T) * appendSize, (&get(originalSize)).address);
    return Status::Success;
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
bool operator==(const Vector<T>& lhs, const Vector<T>& rhs) noexcept {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/// @ingroup ROOT
template <typename T>
bool operator!=(const Vector<T>& lhs, const Vector<T>& rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace pando

#endif // PANDO_RT_CONTAINERS_VECTOR_HPP_
