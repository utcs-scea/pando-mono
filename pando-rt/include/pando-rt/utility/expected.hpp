// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_UTILITY_EXPECTED_HPP_
#define PANDO_RT_UTILITY_EXPECTED_HPP_

#include <type_traits>
#include <utility>
#include <variant>

#include "../status.hpp"

namespace pando {

/**
 * @brief Stores a value or a @ref Status object.
 *
 * @tparam T Expected value type.
 *
 * This is similar to @c std::expected but with additional restrictions:
 * - the error code type is fixed to @ref Status,
 * - an @ref Expected object cannot be default constructed, and
 * - there are no monadic operations.
 *
 * @ingroup ROOT
 */
template <typename T>
class Expected {
  std::variant<T, Status> m_storage;

public:
  template <typename U, std::enable_if_t<!std::is_same_v<std::remove_cv_t<U>, Status>, int> = 0>
  constexpr Expected(U&& value) // NOLINT - implicit conversion allowed
      : m_storage(std::forward<U>(value)) {}

  constexpr Expected(Status status) noexcept // NOLINT - implicit conversion allowed
      : m_storage(status) {}

  constexpr Expected(const Expected&) noexcept = default;
  constexpr Expected(Expected&&) noexcept = default;

  ~Expected() = default;

  Expected& operator=(const Expected&) noexcept = delete;
  Expected& operator=(Expected&&) noexcept = delete;

  /**
   * @brief Returns if there is a value.
   */
  constexpr bool hasValue() const noexcept {
    return m_storage.index() == 0;
  }

  /// @copydoc hasValue()
  constexpr explicit operator bool() const noexcept {
    return hasValue();
  }

  /**
   * @brief Returns the value.
   */
  constexpr T& value() & noexcept {
    return std::get<0>(m_storage);
  }

  /// @copydoc value()
  constexpr const T& value() const& noexcept {
    return std::get<0>(m_storage);
  }

  /// @copydoc value()
  constexpr T&& value() && noexcept {
    return std::move(std::get<0>(m_storage));
  }

  /// @copydoc value()
  constexpr const T&& value() const&& noexcept {
    return std::move(std::get<0>(m_storage));
  }

  /**
   * @brief Returns the error code.
   */
  constexpr Status error() const noexcept {
    return std::get<1>(m_storage);
  }
};

/**
 * @brief Specialization of @ref Expected for @c void.
 *
 * @ingroup ROOT
 */
template <>
class Expected<void> {
  Status m_status = Status::Success;

public:
  constexpr Expected() noexcept = default;

  constexpr Expected(Status status) noexcept // NOLINT - implicit conversion allowed
      : m_status(status) {}

  constexpr Expected(const Expected&) noexcept = default;
  constexpr Expected(Expected&&) noexcept = default;

  ~Expected() = default;

  Expected& operator=(const Expected&) noexcept = delete;
  Expected& operator=(Expected&&) noexcept = delete;

  constexpr bool hasValue() const noexcept {
    return m_status == Status::Success;
  }

  constexpr explicit operator bool() const noexcept {
    return hasValue();
  }

  constexpr Status error() const noexcept {
    return m_status;
  }
};

} // namespace pando

#endif // PANDO_RT_UTILITY_EXPECTED_HPP_
