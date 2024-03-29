// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_PANDO_RT_HPP_
#define PANDO_RT_PANDO_RT_HPP_

/**
 * @defgroup ROOT ROOT
 *
 * The PANDO Runtime System (pando-rt) offers ROOT, the Application Programming Interfaces (APIs) to
 * develop applications for PANDO hardware and its emulators.
 * @{
 */
/**@}*/

#include <cstdint>
#include <cstdio>

#include "execution/bulk_execute_on.hpp"
#include "execution/execute_on.hpp"
#include "execution/execute_on_wait.hpp"
#include "export.h"
#include "index.hpp"
#include "locality.hpp"
#include "main.hpp"
#include "memory/allocate_memory.hpp"
#include "memory/memory_info.hpp"
#include "memory_resource.hpp"
#include "specific_storage.hpp"
#include "status.hpp"
#include "stdlib.hpp"
#include "sync/notification.hpp"
#include "sync/wait.hpp"

/**
 * @brief Checks if the call @p fn was successful or not and exits with message if not.
 *
 * @ingroup ROOT
 */
#define PANDO_CHECK(fn)                                                                    \
  do {                                                                                     \
    const auto status##__LINE__ = (fn);                                                    \
    if (status##__LINE__ != ::pando::Status::Success) {                                    \
      std::fprintf(stderr, "ERROR calling %s (%s:%i): %s (%i)\n", #fn, __FILE__, __LINE__, \
                   ::pando::errorString(status##__LINE__).data(),                          \
                   std::uint32_t(status##__LINE__));                                       \
      std::fflush(stderr);                                                                 \
      ::pando::exit(static_cast<int>(status##__LINE__));                                   \
    }                                                                                      \
  } while (false)

/**
 * @brief Checks if the call @p fn was successful or not and returns if it was not
 *
 * @ingroup ROOT
 */
#define PANDO_CHECK_RETURN(fn)                        \
  do {                                                \
    const auto status##__LINE__ = (fn);               \
    if (status##__LINE__ != pando::Status::Success) { \
      return status##__LINE__;                        \
    }                                                 \
  } while (false)

/**
 * @brief Breaks down an expected type and gives back the inner type or returns a status error
 */
#define PANDO_EXPECT_RETURN(e)          \
  __extension__({                       \
    const auto expect##__LINE__ = e;    \
    if (!expect##__LINE__.hasValue()) { \
      return expect##__LINE__.error();  \
    }                                   \
    expect##__LINE__.value();           \
  })

/**
 * @brief Breaks down an expected type and gives back the inner type or errors out like PANDO_CHECK
 */
#define PANDO_EXPECT_CHECK(e)          \
  __extension__({                       \
    const auto expect##__LINE__ = e;    \
    if (!expect##__LINE__.hasValue()) { \
      PANDO_CHECK(expect##__LINE__.error());  \
    }                                   \
    expect##__LINE__.value();           \
  })

#endif // PANDO_RT_PANDO_RT_HPP_
