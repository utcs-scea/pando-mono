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
 */
double atomicFetchAdd(pando::GlobalPtr<double> ptr, double value, std::memory_order success);
double atomicFetchAdd(pando::GlobalPtr<double> ptr, double value, std::memory_order success,
                      std::memory_order failure);

/**
 * @brief Subtracts a value from a double atomically using compare and swap
 * @warning memoryOrder is ignored, std::memory_order_seq_cst is always used
 */
double atomicFetchSub(pando::GlobalPtr<double> ptr, double value, std::memory_order memoryOrder);
double atomicFetchSub(pando::GlobalPtr<double> ptr, double value, std::memory_order success,
                      std::memory_order failure);

float atomicLoad(pando::GlobalPtr<float> ptr);
float atomicLoad(pando::GlobalPtr<float> ptr, std::memory_order memoryOrder);

float atomicFetchAdd(pando::GlobalPtr<float> ptr, float value);
float atomicFetchSub(pando::GlobalPtr<float> ptr, float value);
/**
 * @brief Adds a value to a float atomically using compare and swap
 */
float atomicFetchAdd(pando::GlobalPtr<float> ptr, float value, std::memory_order success,
                     std::memory_order failure);
/**
 * @brief Subtracts a value from a float atomically using compare and swap
 */
float atomicFetchSub(pando::GlobalPtr<float> ptr, float value, std::memory_order success,
                     std::memory_order failure);

float atomicFetchAdd(pando::GlobalPtr<float> ptr, float value, std::memory_order memoryOrder);
float atomicFetchSub(pando::GlobalPtr<float> ptr, float value, std::memory_order memoryOrder);

} // namespace pando

#endif // PANDO_LIB_GALOIS_SYNC_ATOMIC_HPP_
