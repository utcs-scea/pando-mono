// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
#define PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_

/**
 * @brief lifts a function with no arguments to work on references
 */
#define lift(ref, func)                                                                \
  __extension__({                                                                      \
    auto refComputed##__LINE__ = (ref);                                                \
    typename std::pointer_traits<decltype(&refComputed##__LINE__)>::element_type tmp = \
        refComputed##__LINE__;                                                         \
    auto ret = tmp.func();                                                             \
    refComputed##__LINE__ = tmp;                                                       \
    ret;                                                                               \
  })

/**
 * @brief lifts a function with no arguments to work on a void return type
 */
#define liftVoid(ref, func)                                                            \
  do {                                                                                 \
    auto refComputed##__LINE__ = (ref);                                                \
    typename std::pointer_traits<decltype(&refComputed##__LINE__)>::element_type tmp = \
        refComputed##__LINE__;                                                         \
    tmp.func();                                                                        \
    refComputed##__LINE__ = tmp;                                                       \
  } while (0)

/**
 * @brief maps a function over its arguments up to work on references
 */
#define fmap(ref, func, ...)                                                           \
  __extension__({                                                                      \
    auto refComputed##__LINE__ = (ref);                                                \
    typename std::pointer_traits<decltype(&refComputed##__LINE__)>::element_type tmp = \
        refComputed##__LINE__;                                                         \
    auto ret = tmp.func(__VA_ARGS__);                                                  \
    refComputed##__LINE__ = tmp;                                                       \
    ret;                                                                               \
  })

/**
 * @brief maps a function over it's arguments to work on references and return void
 */
#define fmapVoid(ref, func, ...)                                                            \
  do {                                                                                      \
    auto refComputed##__LINE__ = (ref);                                                     \
    typename std::pointer_traits<decltype(&refComputed##__LINE__)>::element_type tmp = ref; \
    tmp.func(__VA_ARGS__);                                                                  \
    ref = tmp;                                                                              \
  } while (0)

#endif // PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
