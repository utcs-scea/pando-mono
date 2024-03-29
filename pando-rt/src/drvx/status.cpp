// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "status.hpp"

#include <ostream>

namespace pando {

std::ostream& operator<<(std::ostream& os, Status status) {
  return os << errorString(status);
}

} // namespace pando
