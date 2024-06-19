// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_REQUEST_HPP_
#define PANDO_RT_EXECUTION_REQUEST_HPP_

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../index.hpp"
#include "../memory/global_ptr.hpp"
#include "../serialization/archive.hpp"
#include "../status.hpp"
#include "execute_on_impl.hpp"
#include "export.h"
#include "task.hpp"

namespace pando {

namespace detail {

/**
 * @brief Buffer for creating a request.
 *
 * @ingroup ROOT
 */
class RequestBuffer {
  void* m_storage{};
  std::size_t m_size{};
  void* m_metadata{};

public:
  constexpr RequestBuffer() noexcept = default;

  RequestBuffer(const RequestBuffer&) = delete;
  RequestBuffer(RequestBuffer&&) = delete;

  ~RequestBuffer() = default;

  RequestBuffer& operator=(const RequestBuffer&) = delete;
  RequestBuffer& operator=(RequestBuffer&&) = delete;

  /**
   * @brief Allocates @p size bytes for a request to node @p nodeIdx.
   */
  [[nodiscard]] Status acquire(NodeIndex nodeIdx, std::size_t size);

  /**
   * @brief Sends the created request to the node defined in
   *        @ref RequestBuffer::acquire(NodeIndex,std::size_t).
   */
  void release();

  /**
   * @brief Returns a pointer to the space allocated via
   *        @ref RequestBuffer::acquire(NodeIndex,std::size_t).
   */
  void* get() const {
    return m_storage;
  }
};

/**
 * @brief Request base class.
 *
 * A request is a function object that executes on a node. It will call its destructor after the
 * function operator call, so the caller does not need to destroy it.
 *
 * @ingroup ROOT
 */
class Request {
protected:
  using CallableType = Status (*)(Request*);

private:
  CallableType m_f{};

protected:
  constexpr explicit Request(CallableType f) noexcept : m_f{f} {}

  Request(const Request&) = delete;
  Request(Request&&) = delete;

  ~Request() = default;

  Request& operator=(const Request&) = delete;
  Request& operator=(Request&&) = delete;

public:
  /**
   * @brief Executes the request.
   */
  [[nodiscard]] Status operator()() {
    return (*m_f)(this);
  }
};

/**
 * @brief Task request.
 *
 * Instances of this class are used to create tasks that do not return a value.
 *
 * @ingroup ROOT
 */
template <typename F, typename... Args>
class AsyncTaskRequest final : public Request {
  /**
   * @brief Implementation for @ref RequestBase::operator()().
   */
  [[nodiscard]] static Status impl(Request* base) {
    auto self = static_cast<AsyncTaskRequest*>(base);
    auto p = reinterpret_cast<std::byte*>(self) + sizeof(AsyncTaskRequest);
    InputArchive ar(p);

    Place place;
    std::tuple<std::decay_t<F>, std::decay_t<Args>...> t;

    ar(place);
    std::apply(ar, t);

    auto task = std::make_from_tuple<Task>(std::move(t));
    self->~AsyncTaskRequest(); // destroy this object; no members should be accessed after this

    return detail::executeOn(place, std::move(task));
  }

public:
  /**
   * @brief Returns the space required to create an @ref AsyncTaskRequest instance.
   *
   * @param[in] place     where to execute @p f
   * @param[in] f         target to invoke
   * @param[in] args      arguments to pass to @p f
   */
  [[nodiscard]] static std::size_t size(Place place, const F& f, const Args&... args) noexcept {
    SizeArchive ar;
    ar(place, f, args...);
    return sizeof(AsyncTaskRequest) + ar.byteCount();
  }

  /**
   * @brief Creates a new @ref AsyncTaskRequest object.
   *
   * @note The constructor assumes that there is enough space past the end of this object to
   *       store the @p args objects.
   *
   * @param[in] place     where to execute @p f
   * @param[in] f         target to invoke
   * @param[in] args      arguments to pass to @p f
   */
  AsyncTaskRequest(Place place, const F& f, const Args&... args)
      : Request(&impl) {
    auto p = reinterpret_cast<std::byte*>(this) + sizeof(AsyncTaskRequest);
    OutputArchive ar(p);
    ar(place, f, args...);
  }
};

/**
 * @brief Task request with result.
 *
 * Instances of this class are used to create tasks that return a value.
 *
 * @ingroup ROOT
 */
template <typename R, typename F, typename... Args>
class TaskRequest final : public Request {
  /**
   * @brief Implementation for @ref RequestBase::operator()().
   */
  [[nodiscard]] static Status impl(Request* base) {
    auto self = static_cast<TaskRequest*>(base);
    auto p = reinterpret_cast<std::byte*>(self) + sizeof(TaskRequest);
    InputArchive ar(p);

    Place place;
    std::tuple<decltype(Task::withResultPtr), std::decay_t<R>, std::decay_t<F>,
               std::decay_t<Args>...>
        t;

    ar(place);
    std::apply(ar, t);

    auto task = std::make_from_tuple<Task>(std::move(t));
    self->~TaskRequest(); // destroy this object; no members should be accessed after this

    return detail::executeOn(place, std::move(task));
  }

public:
  /**
   * @brief Returns the space required to create an @ref TaskRequest instance.
   *
   * @param[in] place     where to execute @p f
   * @param[in] resultPtr pointer to write the result of `f(args...)`
   * @param[in] f         target to invoke
   * @param[in] args      arguments to pass to @p f
   */
  [[nodiscard]] static std::size_t size(Place place, const R& resultPtr, const F& f,
                                        const Args&... args) noexcept {
    SizeArchive ar;
    ar(place, Task::withResultPtr, resultPtr, f, args...);
    return sizeof(TaskRequest) + ar.byteCount();
  }

  /**
   * @brief Creates a new @ref TaskRequest object.
   *
   * @note The constructor assumes that there is enough space past the end of this object to
   *       store the @p args objects.
   *
   * @param[in] place     where to execute @p f
   * @param[in] resultPtr pointer to write the result of `f(args...)`
   * @param[in] f         target to invoke
   * @param[in] args      arguments to pass to @p f
   */
  TaskRequest(Place place, const R& resultPtr, const F& f, const Args&... args) : Request(&impl) {
    auto p = reinterpret_cast<std::byte*>(this) + sizeof(TaskRequest);
    OutputArchive ar(p);
    ar(place, Task::withResultPtr, resultPtr, f, args...);
  }
};

} // namespace detail

} // namespace pando

#endif //  PANDO_RT_EXECUTION_REQUEST_HPP_
