// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_
#define PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory.hpp>

namespace pando {

double atomicFetchAdd(pando::GlobalPtr<double> ptr, double value);
double atomicFetchSub(pando::GlobalPtr<double> ptr, double value);

} // namespace pando

#endif // PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_
