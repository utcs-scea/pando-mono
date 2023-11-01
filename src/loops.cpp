// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-lib-galois/loops.hpp"

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"

uint64_t galois::dummy() {
  auto core = pando::getCurrentPlace();
  return core.node.id;
}
