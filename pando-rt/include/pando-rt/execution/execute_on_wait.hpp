// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_EXECUTE_ON_WAIT_HPP_
#define PANDO_RT_EXECUTION_EXECUTE_ON_WAIT_HPP_

#include <utility>

#include "../locality.hpp"
#include "../memory.hpp"
#include "../status.hpp"
#include "../sync/wait.hpp"
#include "../utility/expected.hpp"
#include "execute_on_impl.hpp"
#include "result_storage.hpp"
#include "task.hpp"
#ifdef PANDO_RT_USE_BACKEND_PREP
#include "request.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

namespace detail {

/**
 * @brief Implementation of @ref executeOnWait.
 *
 * @param[in] place core to execute @p f in
 * @param[in] f     target to invoke
 * @param[in] args  arguments to pass to @p f
 *
 * @return @ref Expected object with the result of `f(args...)` or an error code
 *
 * @ingroup ROOT
 */
template <typename ResultStorageType, typename F, typename... Args>
[[nodiscard]] Expected<std::invoke_result_t<F, Args...>> executeOnWaitImpl(Place place, F&& f,
                                                                           Args&&... args) {
  ResultStorageType resultStorage;
  if (auto status = resultStorage.initialize(); status != Status::Success) {
    return status;
  }
  auto resultStoragePtr = resultStorage.getPtr();

  // TODO(ashwin #37): until there is load balancing, map anyNode / anyPod to the current node / pod
  if (place.node == anyNode) {
    place.node = getCurrentNode();
  }
  if (place.pod == anyPod) {
    place.pod = (isOnCP() ? PodIndex{0, 0} : getCurrentPod());
  }

#if defined(PANDO_RT_USE_BACKEND_PREP)
  if (place.node == getCurrentNode()) {
    // schedule task on a core on this node
    if (auto status =
            detail::executeOn(place, Task(Task::withResultPtr, resultStoragePtr, std::forward<F>(f),
                                          std::forward<Args>(args)...));
        status != Status::Success) {
      return status;
    }
  } else {
    // allocate space for a task in a remote node
    using RequestType = detail::TaskRequest<decltype(resultStoragePtr), F, Args...>;

    const auto size =
        RequestType::size(place, resultStoragePtr, std::forward<F>(f), std::forward<Args>(args)...);
    detail::RequestBuffer buffer;
    if (auto status = buffer.acquire(place.node, size); status != Status::Success) {
      return status;
    }
    new (buffer.get())
        RequestType(place, resultStoragePtr, std::forward<F>(f), std::forward<Args>(args)...);
    buffer.release();
  }
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  // all PXNs are in the same process in Drv-X, so just execute as though the task was local.
  if (auto status = detail::executeOn(place, Task(Task::withResultPtr, resultStoragePtr,
                                                  std::forward<F>(f), std::forward<Args>(args)...));
      status != Status::Success) {
    return status;
  }
#endif // PANDO_RT_USE_BACKEND_PREP

  // wait for the result
  waitUntil([&resultStorage] {
    return resultStorage.hasValue();
  });

  if constexpr (std::is_same_v<typename ResultStorageType::ValueType, void>) {
    return {};
  } else {
    return resultStorage.moveOutValue();
  }
}

} // namespace detail

/**
 * @brief Executes @p f with arguments @p args on the core in @p place.
 *
 * @param[in] place core to execute @p f in
 * @param[in] f     target to invoke
 * @param[in] args  arguments to pass to @p f
 *
 * @return @ref Expected object with the result of `f(args...)` or an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... Args>
[[nodiscard]] Expected<std::invoke_result_t<F, Args...>> executeOnWait(Place place, F&& f,
                                                                       Args&&... args) {
  using ResultType = std::remove_cv_t<std::invoke_result_t<F, Args...>>;

  if (isOnCP()) {
    using ResultStorageType = detail::AllocatedResultStorage<ResultType>;
    return detail::executeOnWaitImpl<ResultStorageType>(place, std::forward<F>(f),
                                                        std::forward<Args>(args)...);
  } else {
    using ResultStorageType = detail::ResultStorage<ResultType>;
    return detail::executeOnWaitImpl<ResultStorageType>(place, std::forward<F>(f),
                                                        std::forward<Args>(args)...);
  }
}

} // namespace pando

#endif // PANDO_RT_EXECUTION_EXECUTE_ON_WAIT_HPP_
