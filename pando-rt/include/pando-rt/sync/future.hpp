// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#ifndef PANDO_RT_SYNC_FUTURE_HPP_
#define PANDO_RT_SYNC_FUTURE_HPP_

#include "../memory_resource.hpp"
#include "../status.hpp"
#include "wait.hpp"

namespace pando {

/**
 * @brief Abstraction that allows for the storage of the result of asynchronous computations.
 *
 * @ingroup ROOT
 */
template <typename T>
class Future;

/**
 * @brief Specialization of @ref Future for global pointers.
 *
 * @note It performs a point to point synchronization w/o using any memory barrier.
 *
 * @ingroup ROOT
 */
template <typename T>
class Future<GlobalPtr<T>> {
private:
  static inline const GlobalPtr<T> s_errorPtr = globalPtrReinterpretCast<GlobalPtr<T>>(UINTPTR_MAX);

public:
  /**
   * @brief Class to store a value or error from an asynchronous computation.
   */
  class Promise {
    GlobalPtr<GlobalPtr<T>> m_ptr;

    friend class Future;

  public:
    Promise() noexcept = default;

  private:
    explicit constexpr Promise(GlobalPtr<GlobalPtr<T>> ptr) noexcept : m_ptr{ptr} {}

  public:
    /**
     * @brief Signals an event occurrence, and sets the result
     *
     * @warning Calling this function more than once results in undefined behavior.
     */
    void setValue(GlobalPtr<T> val) noexcept {
      *m_ptr = val;
    }

    /**
     * @brief Signals an event has failed, and sets the failure pointer to UINT64_MAX
     *
     * @warning Calling this function more than once results in undefined behavior.
     */
    void setFailure() noexcept {
      *m_ptr = s_errorPtr;
    }
  };

private:
  GlobalPtr<GlobalPtr<T>> m_ptr{};

public:
  /**
   * @brief Constructs a new notification object.
   *
   * @warning The object is not full constructed until one of the `init()` functions is called.
   */
  Future() noexcept = default;

  Future(const Future&) = delete;
  Future(Future&&) = delete;

  ~Future() = default;

  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) = delete;

  /**
   * @brief Initializes this notification object with a user-provided flag.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   */
  [[nodiscard]] Status init(GlobalPtr<GlobalPtr<T>> ptr) noexcept {
    if (ptr == nullptr) {
      return Status::InvalidValue;
    }
    if (m_ptr != nullptr) {
      return Status::AlreadyInit;
    }
    m_ptr = ptr;
    *m_ptr = nullptr;
    return Status::Success;
  }

  /**
   * @brief Initializes this notification object with a user-provided flag, but does not reset it,
   * used to enable multiple waiters to be notified.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   */
  [[nodiscard]] Status initNoReset(GlobalPtr<GlobalPtr<T>> ptr) noexcept {
    if (ptr == nullptr) {
      return Status::InvalidValue;
    }
    if (m_ptr != nullptr) {
      return Status::AlreadyInit;
    }
    m_ptr = ptr;
    return Status::Success;
  }

  /**
   * @brief Returns the handle associated with this notification.
   *
   * This handle is used to signal the occurrence of an event.
   */
  Promise getPromise() const noexcept {
    return Promise{m_ptr};
  }

  /**
   * @brief Waits until one of @ref Promise::setValue or @ref Promise::setFailure is called.
   *
   * @return @c true if there was no error and @c false if there was an error.
   */
  bool wait() {
#ifdef PANDO_RT_USE_BACKEND_PREP
    waitUntil([this] {
      return *m_ptr != nullptr;
    });
#else

#if defined(PANDO_RT_BYPASS)
    if (getBypassFlag()) {
      waitUntil([this] {
        return *m_ptr != nullptr;
      });
    } else {
      DrvAPI::monitor_until_not<GlobalPtr<T>>(m_ptr.address, nullptr);
    }
#else
    DrvAPI::monitor_until_not<GlobalPtr<T>>(m_ptr.address, nullptr);
#endif

#endif
    return s_errorPtr != *m_ptr;
  }
};

/**
 * @brief Alias for @ref Future of a global pointer.
 *
 * @ingroup ROOT
 */
template <typename T>
using PtrFuture = Future<GlobalPtr<T>>;

/**
 * @brief Alias for a Promise within a @ref Future
 *
 * @ingroup ROOT
 */
template <typename T>
using PtrPromise = typename PtrFuture<T>::Promise;

} // namespace pando

#endif //  PANDO_RT_SYNC_FUTURE_HPP_
