// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
#define PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_

/**
 * @brief lifts a function with no arguments to work on references
 */
#define lift(ref, func)                                                   \
  __extension__({                                                         \
    typename std::pointer_traits<decltype(&ref)>::element_type tmp = ref; \
    auto ret = tmp.func();                                                \
    ref = tmp;                                                            \
    ret;                                                                  \
  })

/**
 * @brief lifts a function with no arguments to work on a void return type
 */
#define liftVoid(ref, func)                                               \
  do {                                                                    \
    typename std::pointer_traits<decltype(&ref)>::element_type tmp = ref; \
    tmp.func();                                                           \
    ref = tmp;                                                            \
  } while (0)

/**
 * @brief maps a function over its arguments up to work on references
 */
#define fmap(ref, func, ...)                                              \
  __extension__({                                                         \
    typename std::pointer_traits<decltype(&ref)>::element_type tmp = ref; \
    auto ret = tmp.func(__VA_ARGS__);                                     \
    ref = tmp;                                                            \
    ret;                                                                  \
  })

/**
 * @brief maps a function over it's arguments to work on references and return void
 */
#define fmapVoid(ref, func, ...)                                          \
  do {                                                                    \
    typename std::pointer_traits<decltype(&ref)>::element_type tmp = ref; \
    tmp.func(__VA_ARGS__);                                                \
    ref = tmp;                                                            \
  } while (0)

/**
 * @brief Breaks down an expected type and gives back the inner type or returns a status error
 */
#define PANDO_EXPECT_RETURN(e) \
  __extension__({              \
    if (!e.hasValue()) {       \
      return e.error();        \
    }                          \
    e.value();                 \
  })

#endif // PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
