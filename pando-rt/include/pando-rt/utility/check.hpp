// SPDX-License-Identifier: MIT
/* Copyright (c) 2024 Univserity of Texas at Austin. All rights reserved. */

#ifndef PANDO_RT_UTILITY_CHECK_HPP_
#define PANDO_RT_UTILITY_CHECK_HPP_

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

#endif // PANDO_RT_UTILITY_CHECK_HPP_
