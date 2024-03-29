// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_MEMORY_INFO_HPP_
#define PANDO_RT_MEMORY_MEMORY_INFO_HPP_

#include <cstddef>
#include <utility>

#include "export.h"
#include "global_ptr_fwd.hpp"
#include "memory_type.hpp"

namespace pando {

/**
 * @brief Returns the hart's stack size in bytes.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::size_t getThreadStackSize() noexcept;

/**
 * @brief Returns the hart's available stack in bytes.
 *
 * @warning This function will return an undefined value if called from the CP.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::size_t getThreadAvailableStack() noexcept;

/**
 * @brief Returns the node's L2SP size in bytes.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::size_t getNodeL2SPSize() noexcept;

/**
 * @brief Returns the node's main memory size in bytes.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::size_t getNodeMainMemorySize() noexcept;

namespace detail {

/**
 * @brief Gets the start pointer and size that corresponds to @p memoryType.
 *
 * @param[in] memoryType Memory type to query the start and size for.
 *
 * @return a pair where the first and second elements are the starting pointer and the size of the
 *         segment.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::pair<GlobalPtr<std::byte>, std::size_t> getMemoryStartAndSize(
    MemoryType memoryType) noexcept;

} // namespace detail

} // namespace pando

#endif // PANDO_RT_MEMORY_MEMORY_INFO_HPP_
