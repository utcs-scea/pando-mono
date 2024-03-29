// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/stdlib.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP) || defined(PANDO_RT_USE_BACKEND_DRVX)
#include <cstdlib>
#include <iostream>
#endif

#ifdef PANDO_RT_USE_BACKEND_PREP
#include "prep/nodes.hpp"
#endif

namespace pando {

void exit(int exitCode) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  Nodes::exit(exitCode);

#else

  std::exit(exitCode);

#endif
}

void catastrophicError(std::string_view message, std::string_view file, std::int_least32_t line,
                       std::string_view function) {
#if defined(PANDO_RT_USE_BACKEND_PREP) || defined(PANDO_RT_USE_BACKEND_DRVX)

  std::cerr << file << ':' << line << ' ' << function << ": " << message << '\n';
  std::abort();

#else

#error "Not implemented"

#endif
}

} // namespace pando
