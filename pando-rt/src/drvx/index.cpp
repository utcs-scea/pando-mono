// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "index.hpp"

#include <ostream>

namespace pando {

std::ostream& operator<<(std::ostream& os, NodeIndex nodeIdx) {
  return os << '(' << static_cast<std::int32_t>(nodeIdx.id) << ')';
}

std::ostream& operator<<(std::ostream& os, PodIndex podIdx) {
  return os << '(' << static_cast<std::int32_t>(podIdx.x) << ','
            << static_cast<std::int32_t>(podIdx.y) << ')';
}

std::ostream& operator<<(std::ostream& os, CoreIndex coreIdx) {
  return os << '(' << static_cast<std::int32_t>(coreIdx.x) << ','
            << static_cast<std::int32_t>(coreIdx.y) << ')';
}

std::ostream& operator<<(std::ostream& os, Place place) {
  return os << '(' << place.node << ',' << place.pod << ',' << place.core << ')';
}

std::ostream& operator<<(std::ostream& os, ThreadIndex threadIdx) {
  return os << '(' << static_cast<std::int32_t>(threadIdx.id) << ')';
}

} // namespace pando
