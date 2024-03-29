// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_DATA_TYPE_HPP_
#define PANDO_RT_SRC_PREP_DATA_TYPE_HPP_

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <utility>

#include "log.hpp"

namespace pando {

/**
 * @brief Supported datatypes.
 *
 * @ingroup PREP
 */
enum class DataType : std::int64_t {
  Int8 = 0x0,
  UInt8,
  Int16,
  UInt16,
  Int32,
  UInt32,
  Int64,
  UInt64,
  Count
};

/**
 * @brief Convert @ref DataType to the underlying integral type.
 *
 * @ingroup PREP
 */
constexpr auto operator+(DataType e) noexcept {
  return static_cast<std::underlying_type_t<DataType>>(e);
}

/**
 * @brief Traits for data type @p T.
 *
 * @ingroup PREP
 */
template <typename T>
struct DataTypeTraits;

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::int8_t> {
  using valueType = std::int8_t;
  static constexpr auto dataType = DataType::Int8;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::uint8_t> {
  using valueType = std::uint8_t;
  static constexpr auto dataType = DataType::UInt8;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::int16_t> {
  using valueType = std::int16_t;
  static constexpr auto dataType = DataType::Int16;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::uint16_t> {
  using valueType = std::uint16_t;
  static constexpr auto dataType = DataType::UInt16;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::int32_t> {
  using valueType = std::int32_t;
  static constexpr auto dataType = DataType::Int32;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::uint32_t> {
  using valueType = std::uint32_t;
  static constexpr auto dataType = DataType::UInt32;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::int64_t> {
  using valueType = std::int64_t;
  static constexpr auto dataType = DataType::Int64;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Specialization of @ref DataTypeTraits for @c std::int8_t.
 *
 * @ingroup PREP
 */
template <>
struct DataTypeTraits<std::uint64_t> {
  using valueType = std::uint64_t;
  static constexpr auto dataType = DataType::UInt64;
  static constexpr auto size = sizeof(valueType);
};

/**
 * @brief Dispatches @p f using the data type @p dataType
 *
 * @param[in] dataType data type to dispatch for
 * @param[in] f        function to call
 * @param[in] args     arguments to call @p f with
 *
 * @ingroup PREP
 */
template <typename F, typename... Args>
decltype(auto) dataTypeDispatch(DataType dataType, F&& f, Args&&... args) {
  switch (dataType) {
    case DataType::Int8:
      return std::forward<F>(f).template operator()<std::int8_t>(std::forward<Args>(args)...);
    case DataType::UInt8:
      return std::forward<F>(f).template operator()<std::uint8_t>(std::forward<Args>(args)...);
    case DataType::Int16:
      return std::forward<F>(f).template operator()<std::int16_t>(std::forward<Args>(args)...);
    case DataType::UInt16:
      return std::forward<F>(f).template operator()<std::uint16_t>(std::forward<Args>(args)...);
    case DataType::Int32:
      return std::forward<F>(f).template operator()<std::int32_t>(std::forward<Args>(args)...);
    case DataType::UInt32:
      return std::forward<F>(f).template operator()<std::uint32_t>(std::forward<Args>(args)...);
    case DataType::Int64:
      return std::forward<F>(f).template operator()<std::int64_t>(std::forward<Args>(args)...);
    case DataType::UInt64:
      return std::forward<F>(f).template operator()<std::uint64_t>(std::forward<Args>(args)...);
    default:
      SPDLOG_ERROR("Unknown data type: {}", +dataType);
      std::abort();
  }
}

} // namespace pando

#endif // PANDO_RT_SRC_PREP_DATA_TYPE_HPP_
