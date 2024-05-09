// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_INIT_HPP_
#define PANDO_RT_SRC_INIT_HPP_
#include <pando-rt/benchmark/counters.hpp>

extern Record<std::int64_t> idleCount;

namespace pando {

/**
 * @brief Initializes the PANDO runtime system with software constructs like queues and allocators.
 *
 * This function must not be called by any user code. It is expected to be invoked by the CP thread
 * at PANDO boot time and is applicable to all backend implementations.
 *
 * @ingroup ROOT
 */
void initialize();

/**
 * @brief Finalizes the PANDO runtime system. This function must not be called by any user code.
 *
 * It is expected to be invoked by the CP thread at PANDO shut down time and is applicable to all
 * backend implementations.
 *
 * @ingroup ROOT
 */
void finalize();

} // namespace pando

#endif // PANDO_RT_SRC_INIT_HPP_
