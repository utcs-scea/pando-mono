// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_EXECUTE_ON_HPP_
#define PANDO_RT_EXECUTION_EXECUTE_ON_HPP_

#include <utility>

#include "../locality.hpp"
#include "../status.hpp"
#include "../sync/wait.hpp"
#include "execute_on_impl.hpp"
#ifdef PANDO_RT_USE_BACKEND_PREP
#include "request.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

/**
 * @brief Executes @p f with arguments @p args on the core in @p place.
 *
 * @param[in] place core to execute @p f in
 * @param[in] f     target to invoke
 * @param[in] args  arguments to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... Args>
[[nodiscard]] Status executeOn(Place place, F&& f, Args&&... args) {
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
    return detail::executeOn(place, Task(std::forward<F>(f), std::forward<Args>(args)...));
  } else {
    // allocate space for a task in a remote node
    using RequestType = detail::AsyncTaskRequest<F, Args...>;
    const auto size = RequestType::size(place, std::forward<F>(f), std::forward<Args>(args)...);
    detail::RequestBuffer buffer;
    if (auto status = buffer.acquire(place.node, size); status != Status::Success) {
      return status;
    }
    new (buffer.get()) RequestType(place, std::forward<F>(f), std::forward<Args>(args)...);
    buffer.release();
    return Status::Success;
  }
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  // all PXNs are in the same process in Drv-X, so just execute as though the task was local.
  return detail::executeOn(place, Task(std::forward<F>(f), std::forward<Args>(args)...));
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Executes @p f with arguments @p args on a core in @p podIdx on the current node.
 *
 * @param[in] podIdx pod to execute @p f in
 * @param[in] f      object to execute
 * @param[in] args   arguments to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... Args>
[[nodiscard]] Status executeOn(PodIndex podIdx, F&& f, Args&&... args) {
  // TODO(ashwin #37): until there is load balancing, map anyNode / anyPod to the current node / pod
  if (podIdx == anyPod) {
    podIdx = (isOnCP() ? PodIndex{0, 0} : getCurrentPod());
  }
  const Place place{getCurrentNode(), podIdx, anyCore};
  return detail::executeOn(place, Task(std::forward<F>(f), std::forward<Args>(args)...));
}

/**
 * @brief Executes @p f with arguments @p args on the core @p coreIdx in the current pod.
 *
 * @param[in] coreIdx core to execute @p f in
 * @param[in] f       object to execute
 * @param[in] args    arguments to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... Args>
[[nodiscard]] Status executeOn(CoreIndex coreIdx, F&& f, Args&&... args) {
  // TODO(ashwin #37): until there is load balancing, map anyNode / anyPod to the current node / pod
  const auto podIdx = (isOnCP() ? PodIndex{0, 0} : getCurrentPod());

  const Place place{getCurrentNode(), podIdx, coreIdx};
  return detail::executeOn(place, Task(std::forward<F>(f), std::forward<Args>(args)...));
}

} // namespace pando

#endif // PANDO_RT_EXECUTION_EXECUTE_ON_HPP_
