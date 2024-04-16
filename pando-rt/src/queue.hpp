// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_QUEUE_HPP_
#define PANDO_RT_SRC_QUEUE_HPP_

#include <deque>
#include <mutex>
#include <optional>
#include <utility>

#include "pando-rt/status.hpp"
#include "concurrentqueue.hpp"

namespace pando {

#if defined(PANDO_RT_USE_BACKEND_PREP)
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
  moodycamel::ConcurrentQueue<T> m_queue;

public:
  using ProducerToken = moodycamel::ProducerToken;
  using ConsumerToken = moodycamel::ConsumerToken;
  Queue() : m_queue(10000) {}

  Queue(const Queue&) = delete;
  Queue(Queue&&) = delete;

  ~Queue() = default;

  Queue& operator=(const Queue&) = delete;
  Queue& operator=(Queue&&) = delete;

  /**
   * @brief Enqueues @p t.
   */
  [[nodiscard]] Status enqueue(const T& t) noexcept {
    m_queue.enqueue(t);
    return Status::Success;
  }

  /**
   * @brief Enqueues @p t.
   */
  [[nodiscard]] Status enqueue(ProducerToken & ptok, const T& t) noexcept {
    m_queue.enqueue(ptok);
    return Status::Success;
  }

  /**
   * @copydoc enqueue(const T& t) noexcept
   */
  [[nodiscard]] Status enqueue(T&& t) noexcept {
    m_queue.enqueue(std::forward<T>(t));
    return Status::Success;
  }

  /**
   * @copydoc enqueue(const T& t) noexcept
   */
  [[nodiscard]] Status enqueue(ProducerToken & ptok, T&& t) noexcept {
    m_queue.enqueue(ptok, std::forward<T>(t));
    return Status::Success;
  }

  /**
   * @brief Returns the first element in the queue if it exists.
   */
  std::optional<T> tryDequeue() {
    T potential;
    bool gotItem = m_queue.try_dequeue(potential);
    if (!gotItem) {
      return std::nullopt;
    }
    std::optional<T> o = std::move(potential);
    return o;
  }

  /**
   * @brief Returns the first element in the queue if it exists.
   */
  std::optional<T> tryDequeue(ConsumerToken& ctok) {
    T potential;
    bool gotItem = m_queue.try_dequeue(ctok, potential);
    if (!gotItem) {
      return std::nullopt;
    }
    std::optional<T> o = std::move(potential);
    return o;
  }

  /**
   * @brief Returns if the queue is empty.
   */
  bool empty() const noexcept {
    return 0 == m_queue.size_approx();
  }

  std::uint64_t getApproxSize() const noexcept {
    return m_queue.size_approx();
  }

  /**
   * @brief Clears the queue.
   */
  void clear() noexcept {
    while(!empty()) {
      tryDequeue();
    }
  }

  ProducerToken makeProducerToken() {
    return moodycamel::ProducerToken(m_queue);
  }

  ConsumerToken makeConsumerToken() {
    return moodycamel::ConsumerToken(m_queue);
  }
};
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
template <typename T>
class Queue {
  std::deque<T> m_queue;
  mutable std::mutex m_mutex;

public:
  using ProducerToken = std::uint64_t;
  using ConsumerToken = std::uint64_t;
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
   * @brief Enqueues @p t.
   */
  [[nodiscard]] Status enqueue(ProducerToken &, const T& t) noexcept {
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
   * @copydoc enqueue(const T& t) noexcept
   */
  [[nodiscard]] Status enqueue(ProducerToken &, T&& t) noexcept {
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
   * @brief Returns the first element in the queue if it exists.
   */
  std::optional<T> tryDequeue(ConsumerToken &) {
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
   * @brief Returns the approximate size
   */
  std::uint64_t getApproxSize() const noexcept {
    std::lock_guard lock{m_mutex};
    return m_queue.size();
  }

  /**
   * @brief Clears the queue.
   */
  void clear() noexcept {
    std::lock_guard lock{m_mutex};
    return m_queue.clear();
  }

  ProducerToken makeProducerToken() {
    return 0;
  }

  ConsumerToken makeConsumerToken() {
    return 0;
  }
};
#endif


} // namespace pando

#endif // PANDO_RT_SRC_QUEUE_HPP_
