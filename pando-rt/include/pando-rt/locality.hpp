// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_LOCALITY_HPP_
#define PANDO_RT_LOCALITY_HPP_

#include "export.h"
#include "index.hpp"

namespace pando {

/**
 * @brief Returns the current node index.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT NodeIndex getCurrentNode() noexcept;

/**
 * @brief Returns the node dimensions.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT NodeIndex getNodeDims() noexcept;

/**
 * @brief Returns the current pod index.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT PodIndex getCurrentPod() noexcept;

/**
 * @brief Returns the pod dimensions.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT PodIndex getPodDims() noexcept;

/**
 * @brief Returns the current core index.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT CoreIndex getCurrentCore() noexcept;

/**
 * @brief Returns the core dimensions.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT CoreIndex getCoreDims() noexcept;

/**
 * @brief Returns the current place.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT Place getCurrentPlace() noexcept;

/**
 * @brief Returns the place dimensions.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT Place getPlaceDims() noexcept;

/**
 * @brief Returns the current thread index.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT ThreadIndex getCurrentThread() noexcept;

/**
 * @brief Returns the thread dimensions.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT ThreadIndex getThreadDims() noexcept;

/**
 * @brief Returns if the calling thread is on the CP.
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT bool isOnCP() noexcept;

} // namespace pando

#endif // PANDO_RT_LOCALITY_HPP_
