// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SERIALIZATION_ARCHIVE_HPP_
#define PANDO_RT_SERIALIZATION_ARCHIVE_HPP_

#include <cstddef>
#include <cstring>
#include <memory>
#include <sstream>
#include <type_traits>

#include <cereal/cereal.hpp>

namespace pando {

namespace detail {

/**
 * @brief Returns if @p T is trivially serializable.
 *
 * @ingroup ROOT
 */
template <typename T>
inline constexpr bool IsTriviallySerializable =
    !std::is_pointer_v<T> && !std::is_enum_v<T> &&
    (std::is_trivially_copyable_v<T> || std::is_member_function_pointer_v<T>);

} // namespace detail

/**
 * @brief Output archive that counts required space in bytes.
 *
 * @ingroup ROOT
 */
class SizeArchive : public cereal::OutputArchive<SizeArchive, cereal::AllowEmptyClassElision> {
  std::size_t m_size{};

public:
  SizeArchive() noexcept
      : cereal::OutputArchive<SizeArchive, cereal::AllowEmptyClassElision>(this) {}

  void saveBinary(const void*, std::streamsize size) noexcept {
    m_size += size;
  }

  std::size_t byteCount() const noexcept {
    return m_size;
  }
};

/**
 * @brief Output archive that serializes objects to an already allocated buffer.
 */
class OutputArchive : public cereal::OutputArchive<OutputArchive, cereal::AllowEmptyClassElision> {
  std::byte* m_buffer{};

public:
  explicit OutputArchive(std::byte* buffer) noexcept
      : cereal::OutputArchive<OutputArchive, cereal::AllowEmptyClassElision>(this),
        m_buffer{buffer} {}

  void saveBinary(const void* data, std::streamsize size) noexcept {
    std::memcpy(m_buffer, data, size);
    m_buffer += size;
  }
};

/**
 * @brief Input archive that deserializes objects from a buffer.
 *
 * @ingroup ROOT
 */
class InputArchive : public cereal::InputArchive<InputArchive, cereal::AllowEmptyClassElision> {
  std::byte* m_buffer{};

public:
  explicit InputArchive(std::byte* buffer) noexcept
      : cereal::InputArchive<InputArchive, cereal::AllowEmptyClassElision>(this),
        m_buffer{buffer} {}

  void loadBinary(void* const data, std::streamsize size) noexcept {
    std::memcpy(data, m_buffer, size);
    m_buffer += size;
  }
};

} // namespace pando

namespace cereal {

/**
 * @brief Sizing for trivially serializable types.
 *
 * @note Empty classes are handled by the archive.
 */
template <typename T>
typename std::enable_if_t<pando::detail::IsTriviallySerializable<T> && !std::is_empty_v<T>>
CEREAL_SAVE_FUNCTION_NAME(pando::SizeArchive& ar, const T& t) noexcept {
  ar.saveBinary(std::addressof(t), sizeof(t));
}

/**
 * @brief Saving for trivially serializable types.
 *
 * @note Empty classes are handled by the archive.
 */
template <typename T>
typename std::enable_if_t<pando::detail::IsTriviallySerializable<T> && !std::is_empty_v<T>>
CEREAL_SAVE_FUNCTION_NAME(pando::OutputArchive& ar, const T& t) {
  ar.saveBinary(std::addressof(t), sizeof(t));
}

/**
 * @brief Loading for trivially serializable types.
 *
 * @note Empty classes are handled by the archive.
 */
template <typename T>
typename std::enable_if_t<pando::detail::IsTriviallySerializable<T> && !std::is_empty_v<T>>
CEREAL_LOAD_FUNCTION_NAME(pando::InputArchive& ar, T& t) {
  ar.loadBinary(std::addressof(t), sizeof(t));
}

/**
 *  @brief Sizing for function pointers.
 */
template <typename F>
typename std::enable_if_t<std::is_function_v<F>> CEREAL_SERIALIZE_FUNCTION_NAME(
    pando::SizeArchive& ar, F*& f) {
  ar.saveBinary(std::addressof(f), sizeof(f));
}

/**
 *  @brief Saving for function pointers.
 */
template <typename F>
typename std::enable_if_t<std::is_function_v<F>> CEREAL_SERIALIZE_FUNCTION_NAME(
    pando::OutputArchive& ar, F*& f) {
  ar.saveBinary(std::addressof(f), sizeof(f));
}

/**
 *  @brief Loading for function pointers.
 */
template <typename F>
typename std::enable_if_t<std::is_function_v<F>> CEREAL_SERIALIZE_FUNCTION_NAME(
    pando::InputArchive& ar, F*& f) {
  ar.loadBinary(std::addressof(f), sizeof(f));
}

/**
 *  @brief Sizing for NVP.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::SizeArchive& ar, const NameValuePair<T>& nvp) noexcept {
  ar(nvp.value);
}

/**
 *  @brief Saving for NVP.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::OutputArchive& ar, const NameValuePair<T>& nvp) {
  ar(nvp.value);
}

/**
 *  @brief Loading for NVP.
 */
template <typename T>
void CEREAL_LOAD_FUNCTION_NAME(pando::InputArchive& ar, NameValuePair<T>& nvp) {
  ar(nvp.value);
}

/**
 *  @brief Sizing for @c SizeTag.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::SizeArchive& ar, const SizeTag<T>& t) noexcept {
  ar(t.size);
}

/**
 *  @brief Saving for @c SizeTag.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::OutputArchive& ar, const SizeTag<T>& t) {
  ar(t.size);
}

/**
 *  @brief Loading for @c SizeTag.
 */
template <typename T>
void CEREAL_LOAD_FUNCTION_NAME(pando::InputArchive& ar, SizeTag<T>& t) {
  ar(t.size);
}

/**
 *  @brief Sizing for binary data.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::SizeArchive& ar, const BinaryData<T>& bd) noexcept {
  ar.saveBinary(bd.data, static_cast<std::streamsize>(bd.size));
}

/**
 *  @brief Saving for binary data.
 */
template <typename T>
void CEREAL_SAVE_FUNCTION_NAME(pando::OutputArchive& ar, const BinaryData<T>& bd) {
  ar.saveBinary(bd.data, static_cast<std::streamsize>(bd.size));
}

/**
 *  @brief Loading for binary data.
 */
template <typename T>
void CEREAL_LOAD_FUNCTION_NAME(pando::InputArchive& ar, BinaryData<T>& bd) {
  ar.loadBinary(bd.data, static_cast<std::streamsize>(bd.size));
}

} // namespace cereal

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(pando::SizeArchive)
CEREAL_REGISTER_ARCHIVE(pando::OutputArchive)
CEREAL_REGISTER_ARCHIVE(pando::InputArchive)

// tie input and output archives
CEREAL_SETUP_ARCHIVE_TRAITS(pando::InputArchive, pando::OutputArchive)

#endif //  PANDO_RT_SERIALIZATION_ARCHIVE_HPP_
