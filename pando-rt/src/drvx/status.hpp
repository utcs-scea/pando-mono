// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_DRVX_STATUS_HPP_
#define PANDO_RT_SRC_DRVX_STATUS_HPP_

#include "pando-rt/status.hpp"

#include <spdlog/fmt/ostr.h>

namespace pando {

/// @ingroup DRVX
std::ostream& operator<<(std::ostream&, Status);

} // namespace pando

/// @private
template <>
struct fmt::formatter<pando::Status> : ostream_formatter {};

#endif // PANDO_RT_SRC_DRVX_STATUS_HPP_
