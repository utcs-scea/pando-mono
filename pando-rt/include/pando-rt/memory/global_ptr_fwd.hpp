// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_GLOBAL_PTR_FWD_HPP_
#define PANDO_RT_MEMORY_GLOBAL_PTR_FWD_HPP_

#if defined(PANDO_RT_USE_BACKEND_PREP)

#include <cstdint>

namespace pando {

/// @brief PANDO global address.
using GlobalAddress = std::uint64_t;

/// @private
template <typename T>
struct GlobalPtr;

/// @private
template <typename T>
class GlobalRef;

} // namespace pando

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

#include "DrvAPIAddress.hpp"

namespace pando {

/// @brief PANDO global address.
using GlobalAddress = DrvAPI::DrvAPIAddress;

/// @private
template <typename T>
struct GlobalPtr;

/// @private
template <typename T>
class GlobalRef;

} // namespace pando

#else

namespace pando {

/// @brief PANDO global address.
using GlobalAddress = void*;

/// @private
template <typename T>
using GlobalPtr = T*;

/// @private
template <typename T>
using GlobalRef = T&;

} // namespace pando

#endif // PANDO_RT_USE_BACKEND_PREP

#endif // PANDO_RT_MEMORY_GLOBAL_PTR_FWD_HPP_
