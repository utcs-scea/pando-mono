// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
#define PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_

#define fmap(ref, func, ...)                                              \
  __extension__({                                                         \
    typename std::pointer_traits<decltype(&ref)>::element_type tmp = ref; \
    auto ret = tmp.func(__VA_ARGS__);                                     \
    ref = tmp;                                                            \
    ret;                                                                  \
  })

#endif // PANDO_LIB_GALOIS_UTILITY_GPTR_MONAD_HPP_
