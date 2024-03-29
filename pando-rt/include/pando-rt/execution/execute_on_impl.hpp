// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_EXECUTE_ON_IMPL_HPP_
#define PANDO_RT_EXECUTION_EXECUTE_ON_IMPL_HPP_

#include "../index.hpp"
#include "../status.hpp"
#include "export.h"
#include "task.hpp"

namespace pando {

namespace detail {

/**
 * @brief Executes @p task on core on this node described by @p place.
 *
 * @param[in] place place to execute @p task on
 * @param[in] task  task to execute
 *
 * @return @ref Status::Success upon success, otherwise an error code
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT [[nodiscard]] Status executeOn(Place place, Task task);

} // namespace detail

} // namespace pando

#endif // PANDO_RT_EXECUTION_EXECUTE_ON_IMPL_HPP_
