// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_SPECIFIC_STORAGE_HPP_
#define PANDO_RT_SRC_SPECIFIC_STORAGE_HPP_

#include <cstddef>

#include "pando-rt/memory/memory_type.hpp"
#include "pando-rt/specific_storage.hpp"

namespace pando {

/**
 * @brief Returns the space that is reserved in @p memoryType memory.
 *
 * @ingroup ROOT
 */
std::size_t getReservedMemorySpace(MemoryType memoryType) noexcept;

} // namespace pando

#endif // PANDO_RT_SRC_SPECIFIC_STORAGE_HPP_
