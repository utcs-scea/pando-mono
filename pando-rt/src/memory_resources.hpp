// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_MEMORY_RESOURCES_HPP_
#define PANDO_RT_SRC_MEMORY_RESOURCES_HPP_

#include "pando-rt/memory_resource.hpp"

namespace pando {

/**
 * @brief Initializes memory resources.
 *
 * @ingroup ROOT
 */
void initMemoryResources();

/**
 * @brief Finalizes memory resources.
 *
 * @ingroup ROOT
 */
void finalizeMemoryResources();

} // namespace pando

#endif // PANDO_RT_SRC_MEMORY_RESOURCES_HPP_
