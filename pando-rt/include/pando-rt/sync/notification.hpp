// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */

#ifndef PANDO_RT_SYNC_NOTIFICATION_HPP_
#define PANDO_RT_SYNC_NOTIFICATION_HPP_

#include <chrono>

#include "../memory_resource.hpp"
#include "../status.hpp"
#include "atomic.hpp"
#include "wait.hpp"

namespace pando {

class Notification;
class NotificationArray;

/**
 * @brief Handle associated with this notification for signaling an event occurrence.
 */
class NotificationHandle {
  GlobalPtr<bool> m_flag{};

  constexpr explicit NotificationHandle(GlobalPtr<bool> flag) noexcept : m_flag{flag} {}

  friend class Notification;
  friend class NotificationArray;

public:
  constexpr NotificationHandle() noexcept = default;

  /**
   * @brief Signals an event occurrence.
   *
   * @warning Calling this function more than once results in undefined behavior.
   */
  void notify() noexcept {
    atomicThreadFence(std::memory_order_release);
    *m_flag = true;
  }
};

/**
 * @brief Abstraction that allows the notification of a single event occurrence.
 *
 * @ref Notification objects are used for point-to-point synchronization. It has private state set
 * to `false` upon creation that transitions to `true` at most once.
 *
 * Once a signal for an event has been sent, the @ref Notification object needs to be reset via the
 * @ref Notification::reset function.
 *
 * @note @ref Notification objects are non-copyable and non-movable. To signal for an occurrence, a
 *       copyable and movable handle needs to be acquired via the @ref Notification::getHandle
 *       function.
 *
 * @ingroup ROOT
 */
class Notification {
public:
  using HandleType = NotificationHandle;

private:
  GlobalPtr<bool> m_flag{};
  bool m_ownsFlag{false};

public:
  /**
   * @brief Constructs a new notification object.
   *
   * @warning The object is not full constructed until one of the `init()` functions is called.
   */
  constexpr Notification() noexcept = default;

  Notification(const Notification&) = delete;
  Notification(Notification&&) = delete;

  ~Notification() {
    if (m_ownsFlag) {
      auto memoryResource = pando::getDefaultMainMemoryResource();
      memoryResource->deallocate(m_flag, sizeof(bool));
      m_flag = nullptr;
    }
  }

  Notification& operator=(const Notification&) = delete;
  Notification& operator=(Notification&&) = delete;

  /**
   * @brief Initializes this notification object.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   */
  [[nodiscard]] Status init() noexcept {
    if (m_flag != nullptr) {
      return Status::AlreadyInit;
    }

    // allocate flag
    auto memoryResource = pando::getDefaultMainMemoryResource();
    m_flag = static_cast<pando::GlobalPtr<bool>>(memoryResource->allocate(sizeof(bool)));
    if (m_flag == nullptr) {
      return Status::BadAlloc;
    }
    m_ownsFlag = true;

    reset();

    return Status::Success;
  }

  /**
   * @brief Initializes this notification object with a user-provided flag.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   */
  [[nodiscard]] Status init(GlobalPtr<bool> flag) noexcept {
    if (flag == nullptr) {
      return Status::InvalidValue;
    }

    if (m_flag != nullptr) {
      return Status::AlreadyInit;
    }

    m_flag = flag;
    m_ownsFlag = false;

    reset();

    return Status::Success;
  }

  /**
   * @brief Returns a handle to signal the occurrence of an event.
   */
  constexpr HandleType getHandle() const noexcept {
    return HandleType{m_flag};
  }

  /**
   * @brief Resets the notification object.
   */
  void reset() {
    *m_flag = false;
  }

  /**
   * @brief Returns if the event has occurred.
   */
  bool done() const noexcept {
    if (*m_flag == true) {
      atomicThreadFence(std::memory_order_acquire);
      return true;
    }
    return false;
  }

  /**
   * @brief Waits until @ref HandleType::notify is called.
   */
  void wait() {
    pando::monitorUntil<bool>(m_flag, true);
  }

  /**
   * @brief Waits until @ref HandleType::notify or the timeout expired, whichever happens first.
   *
   * @param[in] timeout timeout period
   *
   * @return @c true if @ref HandleType::notify was called, @c false if the timeout has expired
   */
  template <typename Rep, typename Period>
  [[nodiscard]] bool waitFor(std::chrono::duration<Rep, Period> timeout) {
    using ClockType = std::chrono::steady_clock;

    bool completed = false;
    const auto start = ClockType::now();
    pando::waitUntil([this, &completed, start, timeout] {
      // check if event has occurred
      if (this->done()) {
        completed = true;
        return true;
      }
      // check if timeout expired
      return (std::chrono::steady_clock::now() - start) > timeout;
    });
    return completed;
  }
};

/**
 * @brief Abstraction that allows the notification of a multiple indexed event occurrences.
 *
 * @ref NotificationArray objects are used for many-to-one synchronization. It has private state set
 * to `false` upon creation that transitions to `true` at most once per index.
 *
 * Once a signal for each event has been sent, the @ref NotificationArray object needs to be reset
 * via the @ref NotificationArray::reset function.
 *
 * @note @ref NotificationArray objects are non-copyable and non-movable. To signal for an
 *       occurrence, a copyable and movable handle needs to be acquired via the
 *       @ref NotificationArray::getHandle function.
 *
 * @ingroup ROOT
 */
class NotificationArray {
public:
  using HandleType = NotificationHandle;
  using SizeType = std::uint64_t;

private:
  GlobalPtr<bool> m_flags{};
  SizeType m_size{};
  bool m_ownsFlags{false};

public:
  /**
   * @brief Constructs a new notification array object.
   *
   * @warning The object is not full constructed until one of the `init()` functions is called.
   */
  constexpr NotificationArray() noexcept = default;

  NotificationArray(const NotificationArray&) = delete;
  NotificationArray(NotificationArray&&) = delete;

  ~NotificationArray() {
    if (m_ownsFlags) {
      auto memoryResource = pando::getDefaultMainMemoryResource();
      memoryResource->deallocate(m_flags, m_size * sizeof(bool));
      m_flags = nullptr;
      m_size = 0;
    }
  }

  NotificationArray& operator=(const NotificationArray&) = delete;
  NotificationArray& operator=(NotificationArray&&) = delete;

  /**
   * @brief Initializes this notification array object.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   *
   * @param[in] size array size
   */
  [[nodiscard]] Status init(SizeType size) noexcept {
    if (m_flags != nullptr) {
      return Status::AlreadyInit;
    }

    if (size == 0) {
      return Status::Success;
    }

    // allocate flags
    auto memoryResource = pando::getDefaultMainMemoryResource();
    m_flags = static_cast<pando::GlobalPtr<bool>>(memoryResource->allocate(size * sizeof(bool)));
    if (m_flags == nullptr) {
      return Status::BadAlloc;
    }
    m_size = size;
    m_ownsFlags = true;

    reset();

    return Status::Success;
  }

  /**
   * @brief Initializes this notification object array with user-provided flags.
   *
   * @warning Until this function is called, the object is not fully initialized and calling any
   *          other function is undefined behavior.
   */
  [[nodiscard]] Status init(GlobalPtr<bool> flags, SizeType size) noexcept {
    if (flags == nullptr) {
      return Status::InvalidValue;
    }

    if (m_flags != nullptr) {
      return Status::AlreadyInit;
    }

    m_flags = flags;
    m_size = size;
    m_ownsFlags = false;

    reset();

    return Status::Success;
  }

  /**
   * @brief Returns the array size.
   */
  constexpr SizeType size() const noexcept {
    return m_size;
  }

  /**
   * @brief Returns a handle to signal the occurrence of an event.
   *
   * @param[in] pos index of event caller
   */
  constexpr HandleType getHandle(SizeType pos) const noexcept {
    return HandleType{m_flags + pos};
  }

  /**
   * @brief Resets the notification object.
   */
  void reset() {
    for (SizeType i = 0; i < m_size; ++i) {
      m_flags[i] = false;
    }
  }

  /**
   * @brief Returns if the @c pos event has occurred.
   */
  bool done(SizeType pos) const noexcept {
    if (m_flags[pos] == true) {
      atomicThreadFence(std::memory_order_acquire);
      return true;
    }
    return false;
  }

  /**
   * @brief Returns if all events have occurred.
   */
  bool done() const noexcept {
    for (SizeType i = 0; i < size(); ++i) {
      if (m_flags[i] == false) {
        return false;
      }
    }
    atomicThreadFence(std::memory_order_acquire);
    return true;
  }

  /**
   * @brief Waits until @ref HandleType::notify is called for @c pos.
   */
  void wait(SizeType pos) {
    pando::monitorUntil<bool>(m_flags+pos, true);
  }

  /**
   * @brief Waits until @ref HandleType::notify is called for the whole array.
   */
  void wait() {
    for (SizeType doneIndex = 0; doneIndex < size(); ++doneIndex) {
      pando::monitorUntil<bool>(m_flags+doneIndex, true);
    }
  }

  /**
   * @brief Waits until @ref HandleType::notify or the timeout expired for @p pos, whichever happens
   * first.
   *
   * @param[in] pos     event index
   * @param[in] timeout timeout period
   *
   * @return @c true if @ref HandleType::notify was called, @c false if the timeout has expired
   */
  template <typename Rep, typename Period>
  [[nodiscard]] bool waitFor(SizeType pos, std::chrono::duration<Rep, Period> timeout) {
    using ClockType = std::chrono::steady_clock;

    bool completed = false;
    const auto start = ClockType::now();
    pando::waitUntil([this, pos, start, timeout, &completed]() mutable {
      // check if event occurred
      if (this->done(pos)) {
        completed = true;
        return true;
      }

      // check if timeout has expired
      return (std::chrono::steady_clock::now() - start) > timeout;
    });
    return completed;
  }

  /**
   * @brief Waits until @ref HandleType::notify or the timeout expired, whichever happens first.
   *
   * @param[in] timeout timeout period
   *
   * @return @c true if @ref HandleType::notify was called, @c false if the timeout has expired
   */
  template <typename Rep, typename Period>
  [[nodiscard]] bool waitFor(std::chrono::duration<Rep, Period> timeout) {
    using ClockType = std::chrono::steady_clock;

    SizeType doneIndex = 0;
    bool completed = false;
    const auto start = ClockType::now();
    pando::waitUntil([this, &doneIndex, start, timeout, &completed]() mutable {
      // check all events that have not been checked yet
      for (; doneIndex < this->size(); ++doneIndex) {
        if (m_flags[doneIndex] == false) {
          break;
        }
      }

      // check if all events have occured
      if (doneIndex == this->size()) {
        completed = true;
        atomicThreadFence(std::memory_order_acquire);
        return true;
      }

      // check if timeout has expired
      return (std::chrono::steady_clock::now() - start) > timeout;
    });
    return completed;
  }
};

} // namespace pando

#endif //  PANDO_RT_SYNC_NOTIFICATION_HPP_
