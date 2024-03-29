// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_INDEX_HPP_
#define PANDO_RT_SRC_PREP_INDEX_HPP_

#include <iosfwd>

#include "spdlog/fmt/ostr.h"

#include "pando-rt/index.hpp"

namespace pando {

/// @ingroup PREP
std::ostream& operator<<(std::ostream&, NodeIndex);

/// @ingroup PREP
std::ostream& operator<<(std::ostream&, PodIndex);

/// @ingroup PREP
std::ostream& operator<<(std::ostream&, CoreIndex);

/// @ingroup PREP
std::ostream& operator<<(std::ostream&, Place);

/// @ingroup PREP
std::ostream& operator<<(std::ostream&, ThreadIndex);

} // namespace pando

/// @private
template <>
struct fmt::formatter<pando::NodeIndex> : ostream_formatter {};

/// @private
template <>
struct fmt::formatter<pando::PodIndex> : ostream_formatter {};

/// @private
template <>
struct fmt::formatter<pando::CoreIndex> : ostream_formatter {};

/// @private
template <>
struct fmt::formatter<pando::Place> : ostream_formatter {};

/// @private
template <>
struct fmt::formatter<pando::ThreadIndex> : ostream_formatter {};

#endif // PANDO_RT_SRC_PREP_INDEX_HPP_
