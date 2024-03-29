// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_BULK_EXECUTE_ON_HPP_
#define PANDO_RT_EXECUTION_BULK_EXECUTE_ON_HPP_

#include <utility>

#include "../index.hpp"
#include "../locality.hpp"
#include "../status.hpp"
#include "execute_on.hpp"

namespace pando {

/**
 * @brief Executes @p f with multiple arguments tuples @p argsTuple on the core in @p place.
 *
 * @param[in] place     core to execute @p f in
 * @param[in] f         target to invoke
 * @param[in] argsTuple arguments tuples to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... ArgsTuple>
[[nodiscard]] Status bulkExecuteOn(Place place, F f, ArgsTuple&&... argsTuple) {
  Status ret = Status::Success;
  ((ret = std::apply(
        [&](auto&&... args) {
          return executeOn(place, f, std::forward<decltype(args)>(args)...);
        },
        std::move(argsTuple)),
    ret == Status::Success) &&
   ...);
  return ret;
}

/**
 * @brief Executes @p f with multiple arguments tuples @p argsTuple on a core in pod @p podIdx in
 *        the current node.
 *
 * @param[in] podIdx    pod to execute @p f in
 * @param[in] f         target to invoke
 * @param[in] argsTuple argument tuples to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... ArgsTuple>
[[nodiscard]] Status bulkExecuteOn(PodIndex podIdx, F f, ArgsTuple&&... argsTuple) {
  // TODO(#37): Execute on default core of the pod for now
  const Place place{getCurrentNode(), podIdx, {}};

  Status ret = Status::Success;
  ((ret = std::apply(
        [&](auto&&... args) {
          return executeOn(place, f, std::forward<decltype(args)>(args)...);
        },
        std::move(argsTuple)),
    ret == Status::Success) &&
   ...);
  return ret;
}

/**
 * @brief Executes @p f with multiple argument tuples @p argsTuple on the core @p coreIdx in the
 *        current pod.
 *
 * @param[in] coreIdx   core to execute @p f in
 * @param[in] f         target to invoke
 * @param[in] argsTuple argument tuples to pass to @p f
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
template <typename F, typename... ArgsTuple>
[[nodiscard]] Status bulkExecuteOn(CoreIndex coreIdx, F f, ArgsTuple&&... argsTuple) {
  const Place place{getCurrentNode(), getCurrentPod(), coreIdx};

  Status ret = Status::Success;
  ((ret = std::apply(
        [&](auto&&... args) {
          return executeOn(place, f, std::forward<decltype(args)>(args)...);
        },
        std::move(argsTuple)),
    ret == Status::Success) &&
   ...);
  return ret;
}

} // namespace pando

#endif // PANDO_RT_EXECUTION_BULK_EXECUTE_ON_HPP_
