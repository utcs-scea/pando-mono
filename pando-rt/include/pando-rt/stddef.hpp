// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_STDDEF_HPP_
#define PANDO_RT_STDDEF_HPP_

#include <cstddef>

namespace pando {

/**
 * @brief Trivial standard-layout type whose alignment is as large as all scalar types supported
 *        by the system.
 *
 * @ingroup ROOT
 */
using MaxAlignT = std::max_align_t;

} // namespace pando

#endif // PANDO_RT_STDDEF_HPP_
