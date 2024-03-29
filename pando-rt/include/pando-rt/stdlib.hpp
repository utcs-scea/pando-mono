// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_STDLIB_HPP_
#define PANDO_RT_STDLIB_HPP_

#include <cstdint>
#include <string_view>

#include "export.h"

namespace pando {

/**
 * @brief Exits the application with @p exitCode as the error code.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT [[noreturn]] void exit(int exitCode);

/**
 * @brief PANDO runtime internal error reporting function.
 *
 * @note This function will abort execution instead of attempting to do any cleanup.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT [[noreturn]] void catastrophicError(std::string_view message, std::string_view file,
                                                    std::int_least32_t line,
                                                    std::string_view function);

} // namespace pando

/**
 * @brief Aborts execution with @p message.
 *
 * @ingroup ROOT
 */
#define PANDO_ABORT(message)                                                        \
  do {                                                                              \
    ::pando::catastrophicError((message), __FILE__, __LINE__, __PRETTY_FUNCTION__); \
  } while (false)

#endif // PANDO_RT_STDLIB_HPP_
