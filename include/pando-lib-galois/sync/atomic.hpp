// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_
#define PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_

#include <pando-rt/export.h>

#include <pando-rt/memory.hpp>

namespace pando {

double atomicLoad(pando::GlobalPtr<double> ptr);
double atomicLoad(pando::GlobalPtr<double> ptr, std::memory_order memoryOrder);

double atomicFetchAdd(pando::GlobalPtr<double> ptr, double value);
double atomicFetchSub(pando::GlobalPtr<double> ptr, double value);
/**
 * @brief Adds a value to a double atomically using compare and swap
 * @warning memoryOrder is ignored, std::memory_order_seq_cst is always used
 */
double atomicFetchAdd(pando::GlobalPtr<double> ptr, double value, std::memory_order memoryOrder);
/**
 * @brief Subtracts a value from a double atomically using compare and swap
 * @warning memoryOrder is ignored, std::memory_order_seq_cst is always used
 */
double atomicFetchSub(pando::GlobalPtr<double> ptr, double value, std::memory_order memoryOrder);

} // namespace pando

#endif // PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_
