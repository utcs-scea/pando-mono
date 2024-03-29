// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_RESULT_STORAGE_HPP_
#define PANDO_RT_EXECUTION_RESULT_STORAGE_HPP_

#include <cstddef>
#include <utility>

#include "../memory.hpp"
#include "../memory_resource.hpp"
#include "../sync/atomic.hpp"

namespace pando {

namespace detail {

/**
 * @brief Storage for object of type @p T.
 *
 * Objects of this class are ephemeral objects that are used to store return values prior to
 * returning them to the caller.
 *
 * @ingroup ROOT
 */
template <typename T>
struct ResultStorage {
  using ValueType = T;

  alignas(T) std::byte data[sizeof(T)];
  std::int32_t ready{0};

  constexpr ResultStorage() noexcept = default;

  ResultStorage(const ResultStorage&) = delete;
  ResultStorage(ResultStorage&&) = delete;

  ~ResultStorage() {
    std::destroy_at(reinterpret_cast<T*>(data));
  }

  ResultStorage& operator=(const ResultStorage&) = delete;
  ResultStorage& operator=(ResultStorage&&) = default;

  [[nodiscard]] Status initialize() {
    return Status::Success;
  }

  auto getPtr() noexcept {
    return GlobalPtr<ResultStorage>(this);
  }

  bool hasValue() const noexcept {
    return atomicLoad(&ready, std::memory_order_acquire) == 1;
  }

  T moveOutValue() noexcept {
    return std::move(*reinterpret_cast<T*>(data));
  }
};

/**
 * @brief Specialization of @ref ResultStorage for @c void.
 *
 * @ingroup ROOT
 */
template <>
struct ResultStorage<void> {
  using ValueType = void;

  std::int32_t ready{0};

  constexpr ResultStorage() noexcept = default;

  ResultStorage(const ResultStorage&) = delete;
  ResultStorage(ResultStorage&&) = delete;

  ~ResultStorage() = default;

  ResultStorage& operator=(const ResultStorage&) = delete;
  ResultStorage& operator=(ResultStorage&&) = default;

  [[nodiscard]] Status initialize() {
    return Status::Success;
  }

  auto getPtr() noexcept {
    return GlobalPtr<ResultStorage>(this);
  }

  bool hasValue() const noexcept {
    return atomicLoad(&ready, std::memory_order_acquire) == 1;
  }
};

/**
 * @brief Dynamically allocated storage for object of type @p T.
 *
 * Objects of this class are ephemeral objects that are used to store return values prior to
 * returning them to the caller.
 *
 * @ingroup ROOT
 */
template <typename T>
class AllocatedResultStorage {
public:
  using ValueType = T;
  using StorageType = ResultStorage<T>;

private:
  GlobalPtr<StorageType> m_storage{};

public:
  constexpr AllocatedResultStorage() noexcept = default;

  AllocatedResultStorage(const AllocatedResultStorage&) = delete;
  AllocatedResultStorage(AllocatedResultStorage&&) = delete;

  ~AllocatedResultStorage() {
    if (m_storage != nullptr) {
      destroyAt(m_storage);
      auto memoryResource = getDefaultMainMemoryResource();
      memoryResource->deallocate(m_storage, sizeof(StorageType));
    }
  }

  AllocatedResultStorage& operator=(const AllocatedResultStorage&) = delete;
  AllocatedResultStorage& operator=(AllocatedResultStorage&&) = delete;

  [[nodiscard]] Status initialize() {
    auto memoryResource = getDefaultMainMemoryResource();
    m_storage = static_cast<GlobalPtr<StorageType>>(memoryResource->allocate(sizeof(StorageType)));
    if (m_storage == nullptr) {
      return Status::BadAlloc;
    }
    constructAt(m_storage);
    return Status::Success;
  }

  GlobalPtr<StorageType> getPtr() noexcept {
    return m_storage;
  }

  bool hasValue() const noexcept {
    auto readyPtr = memberPtrOf<std::int32_t>(m_storage, offsetof(StorageType, ready));
    return atomicLoad(readyPtr, std::memory_order_acquire) == 1;
  }

  auto moveOutValue() noexcept {
    auto ptr = memberPtrOf<T>(m_storage, offsetof(StorageType, data));
    return *ptr;
  }
};

} // namespace detail

} // namespace pando

#endif // PANDO_RT_EXECUTION_RESULT_STORAGE_HPP_
