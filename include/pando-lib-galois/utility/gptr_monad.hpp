// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
#define PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
/**
 * @brief lifts a function with no arguments to work on references
 */

#define lift(ref, func)                                                              \
  __extension__({                                                                    \
    auto ptrComputed##__LINE__ = &(ref);                                             \
    using ElementType =                                                              \
        typename std::pointer_traits<decltype(ptrComputed##__LINE__)>::element_type; \
    auto ret = (strcmp(#func, "size") == 0) ?                               \
      [&]() {  \
        ElementType tmp;  \
        using ReturnType = decltype(tmp.func()); \
        ReturnType test = \
        pando::GlobalPtr<ReturnType> (ptrComputed##__LINE__  + 4);       \
        return test; \
        }() :                           \
      [&]() {                                                       \
        ElementType tmp = *ptrComputed##__LINE__;                                     \
        *ptrComputed##__LINE__ = tmp;                                                 \
        return tmp.func();                                                            \
      }();        \
    ret;                                                                             \
  })

/**
 * @brief lifts a function with no arguments to work on a void return type
 */
#define liftVoid(ref, func)                                                           \
  do {                                                                                \
    auto ptrComputed##__LINE__ = &(ref);                                              \
    typename std::pointer_traits<decltype(ptrComputed##__LINE__)>::element_type tmp = \
        *ptrComputed##__LINE__;                                                       \
    tmp.func();                                                                       \
    *ptrComputed##__LINE__ = tmp;                                                     \
  } while (0)

/**
 * @brief maps a function over its arguments up to work on references
 */
#define fmap(ref, func, ...)                                                          \
  __extension__({                                                                     \
    auto ptrComputed##__LINE__ = &(ref);                                              \
    typename std::pointer_traits<decltype(ptrComputed##__LINE__)>::element_type tmp = \
        *ptrComputed##__LINE__;                                                       \
    auto ret = tmp.func(__VA_ARGS__);                                                 \
    *ptrComputed##__LINE__ = tmp;                                                     \
    ret;                                                                              \
  })

/**
 * @brief maps a function over it's arguments to work on references and return void
 */
#define fmapVoid(ref, func, ...)                                                      \
  do {                                                                                \
    auto ptrComputed##__LINE__ = &(ref);                                              \
    typename std::pointer_traits<decltype(ptrComputed##__LINE__)>::element_type tmp = \
        *ptrComputed##__LINE__;                                                       \
    tmp.func(__VA_ARGS__);                                                            \
    *ptrComputed##__LINE__ = tmp;                                                     \
  } while (0)

#endif // PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
