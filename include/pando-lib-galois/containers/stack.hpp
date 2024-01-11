// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <utility>

#include "pando-rt/export.h"

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

#ifndef PANDO_LIB_GALOIS_CONTAINERS_STACK_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_STACK_HPP_

namespace galois {

/**
 * @brief Standard single-threaded stack that stores elements of type @p T contiguously and can
 * change size dynamically.
 *
 * @note A @c Stack object is empty upon construction. One of the `Stack::initialize()` functions
 *       needs to be called to allocate space.
 */
template <typename T>
class Stack {
private:
  /// @brief The size of the stack
  std::uint64_t m_size = 0;
  /// @brief An array that holds the data
  pando::Array<T> m_buf;

  /**
   * @brief Reserves space in the container for at least @p nextCapacity number of elements.
   *
   * @note If the new capacity is less that the current capacity, the function has not effect.
   * @note The function does not change the size of the container and does not initialize the new
   *       elements.
   *
   * @param[in] nextCapacity new capacity of the container in elements
   */
  pando::Status reserve(std::uint64_t nextCapacity) {
    if (nextCapacity <= capacity()) {
      return pando::Status::Success;
    }

    pando::Array<T> newArray;
    auto status =
        newArray.initialize(nextCapacity, pando::localityOf(m_buf.data()), m_buf.getMemoryType());
    PANDO_CHECK_RETURN(status);

    for (std::uint64_t i = 0; i < size(); i++) {
      newArray[i] = std::move<T>(m_buf[i]);
    }

    std::swap(m_buf, newArray);
    newArray.deinitialize();

    return pando::Status::Success;
  }

  /**
   * @brief Reserves the current capacity * 2
   */
  pando::Status grow() {
    if (m_buf.data() == nullptr) {
      return pando::Status::NotInit;
    }
    return reserve(m_buf.size() * 2);
  }

public:
  Stack() = default;

  /**
   * @copydoc initialize(std::uint64_t)
   *
   * @param[in] place      place to allocate memory from
   * @param[in] memoryType memory to allocate from
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size, pando::Place place,
                                         pando::MemoryType memoryType) {
    if (size < 1) {
      size = 1;
    }
    const auto status = m_buf.initialize(size, place, memoryType);
    m_size = 0;
    return status;
  }

  /**
   * @brief Initializes the vector with the given size in @ref MemoryType::Main memory.
   *
   * @note This function will default initialize all elements.
   *
   * @param[in] size size of vector in elements
   */
  [[nodiscard]] pando::Status initialize(std::uint64_t size) {
    return initialize(size, pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  /**
   * @brief Deinitializes the container.
   */
  void deinitialize() {
    m_buf.deinitialize();
    m_size = 0;
  }

  bool empty() const {
    return size() == 0;
  }

  size_t size() const {
    return m_size;
  }

  size_t capacity() const {
    return m_buf.size();
  }

  [[nodiscard]] pando::Status emplace(T elt) {
    if (m_size >= m_buf.size()) {
      pando::Status err = grow();
      PANDO_CHECK_RETURN(err);
    }
    m_buf[m_size++] = elt;
    return pando::Status::Success;
  }

  [[nodiscard]] pando::Status pop(T& elt) {
    if (empty()) {
      return pando::Status::OutOfBounds;
    }
    elt = m_buf[--m_size];
    return pando::Status::Success;
  }
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_STACK_HPP_
