// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SPECIFIC_STORAGE_HPP_
#define PANDO_RT_SPECIFIC_STORAGE_HPP_

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include "export.h"
#include "locality.hpp"
#include "memory/global_ptr.hpp"
#include "memory/memory_type.hpp"
#include "memory_resource.hpp"
#include "utility/expected.hpp"

namespace pando {

namespace detail {

/**
 * @brief Reserves @p size bytes in @ref MemoryType::L2SP memory that will be zero-initialized.
 *
 * @param[in] size       bytes to reserve
 * @param[in] alignment  requested alignment
 *
 * @return offset in the memory
 */
PANDO_RT_EXPORT std::size_t reserveZeroInitL2SPMemory(std::size_t size, std::size_t alignment);

/**
 * @brief Reserves @p size bytes in @ref MemoryType::Main memory that will be zero-initialized.
 *
 * @param[in] size       bytes to reserve
 * @param[in] alignment  requested alignment
 *
 * @return offset in the memory
 */
PANDO_RT_EXPORT std::size_t reserveZeroInitMainMemory(std::size_t size, std::size_t alignment);

} // namespace detail

template <typename T>
class PodSpecificStorage;

template <typename T>
class PodSpecificStorageAlias {
public:
  using value_type = T;
  friend PodSpecificStorage<T>;
  template <typename Y>
  friend class PodSpecificStorageAlias;

private:
  std::size_t m_offset = 0;

public:
  PodSpecificStorageAlias() {}

  PodSpecificStorageAlias(const PodSpecificStorageAlias&) = default;
  PodSpecificStorageAlias(PodSpecificStorageAlias&&) = default;

  ~PodSpecificStorageAlias() = default;

  PodSpecificStorageAlias& operator=(const PodSpecificStorageAlias&) = default;
  PodSpecificStorageAlias& operator=(PodSpecificStorageAlias&&) = default;

  PodSpecificStorageAlias& operator=(const T& t) {
    *getPointer() = t;
    return *this;
  }

  PodSpecificStorageAlias& operator=(T&& t) {
    *getPointer() = std::move(t);
    return *this;
  }

private:
  explicit PodSpecificStorageAlias(std::size_t offset) : m_offset(offset) {}

  GlobalAddress getAddress() const noexcept {
    if (isOnCP()) {
      PANDO_ABORT("Address cannot be inferred by the CP");
    }
    return encodeL2SPAddress(getCurrentNode(), getCurrentPod(), m_offset);
  }

  GlobalAddress getAddressAt(NodeIndex nodeIdx, PodIndex podIdx) const noexcept {
    if ((nodeIdx < NodeIndex(0) || nodeIdx >= getNodeDims()) ||
        (podIdx < PodIndex(0, 0) || podIdx >= getPodDims())) {
      PANDO_ABORT("Address beyond addressable range");
    }
    return encodeL2SPAddress(nodeIdx, podIdx, m_offset);
  }

public:
  /**
   * @brief Returns a pointer to this pod's instance of @c T.
   */
  GlobalPtr<T> getPointer() noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddress());
  }

  /// @copydoc getPointer()
  GlobalPtr<const T> getPointer() const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddress());
  }

  /**
   * @brief Returns a pointer to the instance at node @p nodeIdx and pod @p podIdx.
   */
  GlobalPtr<T> getPointerAt(NodeIndex nodeIdx, PodIndex podIdx) noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddressAt(nodeIdx, podIdx));
  }

  /// @copydoc getPointerAt(NodeIndex,PodIndex)
  GlobalPtr<const T> getPointerAt(NodeIndex nodeIdx, PodIndex podIdx) const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddressAt(nodeIdx, podIdx));
  }

  operator T() const noexcept {
    return *getPointer();
  }

  GlobalPtr<T> operator&() noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  GlobalPtr<const T> operator&() const noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  template <typename Y>
  Expected<PodSpecificStorageAlias<Y>> getStorageAliasAt(GlobalPtr<Y> currPtr) {
    NodeIndex nodeIdx = pando::localityOf(currPtr).node;
    PodIndex podIdx = pando::localityOf(currPtr).pod;
    GlobalPtr<std::byte> startByte = static_cast<GlobalPtr<std::byte>>(
        static_cast<GlobalPtr<void>>(this->getPointerAt(nodeIdx, podIdx)));
    GlobalPtr<std::byte> currsByte =
        static_cast<GlobalPtr<std::byte>>(static_cast<GlobalPtr<void>>(currPtr));
    if (sizeof(T) <= sizeof(Y) || startByte > currsByte || startByte + sizeof(T) <= currsByte ||
        startByte + sizeof(T) < currsByte + sizeof(Y)) {
      return Status::OutOfBounds;
    }
    std::size_t diff = currsByte - startByte;
    return PodSpecificStorageAlias<Y>(this->m_offset + diff);
  }
};

/**
 * @brief Storage specific to a pod.
 *
 * Each pod will have an instance of @c T that is globally accessible. All instances are
 * zero-initialized.
 *
 * @ingroup ROOT
 */
template <typename T>
class PodSpecificStorage {
  static_assert(std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>,
                "T must be trivially constructible and destructible");
  friend PodSpecificStorageAlias<T>;

public:
  using value_type = T;

private:
  std::size_t m_offset;

public:
  PodSpecificStorage() : m_offset(detail::reserveZeroInitL2SPMemory(sizeof(T), alignof(T))) {}

  PodSpecificStorage(const PodSpecificStorage&) = delete;
  PodSpecificStorage(PodSpecificStorage&&) = delete;

  ~PodSpecificStorage() = default;

  PodSpecificStorage& operator=(const PodSpecificStorage&) = delete;
  PodSpecificStorage& operator=(PodSpecificStorage&&) = delete;

  PodSpecificStorage& operator=(const T& t) {
    *getPointer() = t;
    return *this;
  }

  PodSpecificStorage& operator=(T&& t) {
    *getPointer() = std::move(t);
    return *this;
  }

  operator PodSpecificStorageAlias<T>() const {
    return PodSpecificStorageAlias<T>(m_offset);
  }

private:
  GlobalAddress getAddress() const noexcept {
    if (isOnCP()) {
      PANDO_ABORT("Address cannot be inferred by the CP");
    }
    return encodeL2SPAddress(getCurrentNode(), getCurrentPod(), m_offset);
  }

  GlobalAddress getAddressAt(NodeIndex nodeIdx, PodIndex podIdx) const noexcept {
    if ((nodeIdx < NodeIndex(0) || nodeIdx >= getNodeDims()) ||
        (podIdx < PodIndex(0, 0) || podIdx >= getPodDims())) {
      PANDO_ABORT("Address beyond addressable range");
    }
    return encodeL2SPAddress(nodeIdx, podIdx, m_offset);
  }

public:
  /**
   * @brief Returns a pointer to this pod's instance of @c T.
   */
  GlobalPtr<T> getPointer() noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddress());
  }

  /// @copydoc getPointer()
  GlobalPtr<const T> getPointer() const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddress());
  }

  /**
   * @brief Returns a pointer to the instance at node @p nodeIdx and pod @p podIdx.
   */
  GlobalPtr<T> getPointerAt(NodeIndex nodeIdx, PodIndex podIdx) noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddressAt(nodeIdx, podIdx));
  }

  /// @copydoc getPointerAt(NodeIndex,PodIndex)
  GlobalPtr<const T> getPointerAt(NodeIndex nodeIdx, PodIndex podIdx) const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddressAt(nodeIdx, podIdx));
  }

  operator T() const noexcept {
    return *getPointer();
  }

  GlobalPtr<T> operator&() noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  GlobalPtr<const T> operator&() const noexcept { // NOLINT - global pointer support
    return getPointer();
  }
};

template <typename T>
class NodeSpecificStorage;

/**
 * @brief Storage specific to a node used as an alias
 *
 * This is a passable version of this storage
 *
 * @ingroup ROOT
 */
template <typename T>
class NodeSpecificStorageAlias {
public:
  using value_type = T;
  friend NodeSpecificStorage<T>;
  template <typename Y>
  friend class NodeSpecificStorageAlias;

private:
  std::size_t m_offset = 0;

public:
  NodeSpecificStorageAlias() {}

  NodeSpecificStorageAlias(const NodeSpecificStorageAlias&) = default;
  NodeSpecificStorageAlias(NodeSpecificStorageAlias&&) = default;

  ~NodeSpecificStorageAlias() = default;

  NodeSpecificStorageAlias& operator=(const NodeSpecificStorageAlias&) = default;
  NodeSpecificStorageAlias& operator=(NodeSpecificStorageAlias&&) = default;

  NodeSpecificStorageAlias& operator=(const T& t) {
    *getPointer() = t;
    return *this;
  }

  NodeSpecificStorageAlias& operator=(T&& t) {
    *getPointer() = std::move(t);
    return *this;
  }

private:
  explicit NodeSpecificStorageAlias(std::size_t offset) : m_offset(offset) {}

  GlobalAddress getAddress() const noexcept {
    return encodeMainAddress(getCurrentNode(), m_offset);
  }

  GlobalAddress getAddressAt(NodeIndex nodeIdx) const noexcept {
    if (nodeIdx < NodeIndex(0) || nodeIdx >= getNodeDims()) {
      PANDO_ABORT("Address beyond addressable range");
    }
    return encodeMainAddress(nodeIdx, m_offset);
  }

public:
  /**
   * @brief Returns a pointer to this node's instance of @c T.
   */
  GlobalPtr<T> getPointer() noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddress());
  }

  /// @copydoc getPointer()
  GlobalPtr<const T> getPointer() const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddress());
  }

  /**
   * @brief Returns a pointer to the instance at node @p nodeIdx.
   */
  GlobalPtr<T> getPointerAt(NodeIndex nodeIdx) noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddressAt(nodeIdx));
  }

  /// @copydoc getPointerAt(NodeIndex)
  GlobalPtr<const T> getPointerAt(NodeIndex nodeIdx) const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddressAt(nodeIdx));
  }

  operator T() const noexcept {
    return *getPointer();
  }

  GlobalPtr<T> operator&() noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  GlobalPtr<const T> operator&() const noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  template <typename Y>
  Expected<NodeSpecificStorageAlias<Y>> getStorageAliasAt(GlobalPtr<Y> currPtr) {
    NodeIndex nodeIdx = pando::localityOf(currPtr).node;
    GlobalPtr<std::byte> startByte = static_cast<GlobalPtr<std::byte>>(
        static_cast<GlobalPtr<void>>(this->getPointerAt(nodeIdx)));
    GlobalPtr<std::byte> currsByte =
        static_cast<GlobalPtr<std::byte>>(static_cast<GlobalPtr<void>>(currPtr));
    if (sizeof(T) <= sizeof(Y) || startByte > currsByte || startByte + sizeof(T) <= currsByte ||
        startByte + sizeof(T) < currsByte + sizeof(Y)) {
      return Status::OutOfBounds;
    }
    std::size_t diff = currsByte - startByte;
    return NodeSpecificStorageAlias<Y>(this->m_offset + diff);
  }
};

/**
 * @brief Storage specific to a node.
 *
 * Each node will have an instance of @c T that is globally accessible. All instances are
 * zero-initialized.
 *
 * @ingroup ROOT
 */
template <typename T>
class NodeSpecificStorage {
  static_assert(std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>,
                "T must be trivially constructible and destructible");
  friend NodeSpecificStorageAlias<T>;

public:
  using value_type = T;

private:
  std::size_t m_offset;

public:
  NodeSpecificStorage() : m_offset(detail::reserveZeroInitMainMemory(sizeof(T), alignof(T))) {}

  NodeSpecificStorage(const NodeSpecificStorage&) = delete;
  NodeSpecificStorage(NodeSpecificStorage&&) = delete;

  ~NodeSpecificStorage() = default;

  NodeSpecificStorage& operator=(const NodeSpecificStorage&) = delete;
  NodeSpecificStorage& operator=(NodeSpecificStorage&&) = delete;

  operator NodeSpecificStorageAlias<T>() const {
    return NodeSpecificStorageAlias<T>(m_offset);
  }

  NodeSpecificStorage& operator=(const T& t) {
    *getPointer() = t;
    return *this;
  }

  NodeSpecificStorage& operator=(T&& t) {
    *getPointer() = std::move(t);
    return *this;
  }

private:
  GlobalAddress getAddress() const noexcept {
    return encodeMainAddress(getCurrentNode(), m_offset);
  }

  GlobalAddress getAddressAt(NodeIndex nodeIdx) const noexcept {
    if (nodeIdx < NodeIndex(0) || nodeIdx >= getNodeDims()) {
      PANDO_ABORT("Address beyond addressable range");
    }
    return encodeMainAddress(nodeIdx, m_offset);
  }

public:
  /**
   * @brief Returns a pointer to this node's instance of @c T.
   */
  GlobalPtr<T> getPointer() noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddress());
  }

  /// @copydoc getPointer()
  GlobalPtr<const T> getPointer() const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddress());
  }

  /**
   * @brief Returns a pointer to the instance at node @p nodeIdx.
   */
  GlobalPtr<T> getPointerAt(NodeIndex nodeIdx) noexcept {
    return globalPtrReinterpretCast<GlobalPtr<T>>(getAddressAt(nodeIdx));
  }

  /// @copydoc getPointerAt(NodeIndex)
  GlobalPtr<const T> getPointerAt(NodeIndex nodeIdx) const noexcept {
    return globalPtrReinterpretCast<GlobalPtr<const T>>(getAddressAt(nodeIdx));
  }

  operator T() const noexcept {
    return *getPointer();
  }

  GlobalPtr<T> operator&() noexcept { // NOLINT - global pointer support
    return getPointer();
  }

  GlobalPtr<const T> operator&() const noexcept { // NOLINT - global pointer support
    return getPointer();
  }
};

} // namespace pando

#endif // PANDO_RT_SPECIFIC_STORAGE_HPP_
