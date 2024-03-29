// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_QUEUE_HPP_
#define PANDO_RT_SRC_QUEUE_HPP_

#include <deque>
#include <mutex>
#include <optional>
#include <utility>

#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Simple thread-safe queue.
 *
 * @note This is temporary queue until the circular buffer is implemented. It is neither generic nor
 * efficient.
 *
 * @ingroup PREP
 */
template <typename T>
class Queue {
  std::deque<T> m_queue;
  mutable std::mutex m_mutex;

public:
  Queue() = default;

  Queue(const Queue&) = delete;
  Queue(Queue&&) = delete;

  ~Queue() = default;

  Queue& operator=(const Queue&) = delete;
  Queue& operator=(Queue&&) = delete;

  /**
   * @brief Enqueues @p t.
   */
  [[nodiscard]] Status enqueue(const T& t) noexcept {
    std::lock_guard lock{m_mutex};
    m_queue.push_back(t);
    return Status::Success;
  }

  /**
   * @copydoc enqueue(const T& t) noexcept
   */
  [[nodiscard]] Status enqueue(T&& t) noexcept {
    std::lock_guard lock{m_mutex};
    m_queue.push_back(std::move(t));
    return Status::Success;
  }

  /**
   * @brief Returns the first element in the queue if it exists.
   */
  std::optional<T> tryDequeue() {
    std::lock_guard lock{m_mutex};
    if (m_queue.empty()) {
      return std::nullopt;
    }
    std::optional<T> o = std::move(m_queue.front());
    m_queue.pop_front();
    return o;
  }

  /**
   * @brief Returns if the queue is empty.
   */
  bool empty() const noexcept {
    std::lock_guard lock{m_mutex};
    return m_queue.empty();
  }

  /**
   * @brief Clears the queue.
   */
  void clear() noexcept {
    std::lock_guard lock{m_mutex};
    return m_queue.clear();
  }
};

} // namespace pando

#endif // PANDO_RT_SRC_QUEUE_HPP_
