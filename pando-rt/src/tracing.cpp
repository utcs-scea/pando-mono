// SPDX-License-Identifier: MIT
/* Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/tracing.hpp"

#include "prep/memtrace_stat.hpp"
#include <pando-rt/locality.hpp>

namespace pando {

void memStatNewPhase() {
  MemTraceStat::writePhase();
}

void memStatNewKernel(std::string_view kernelName) {
  MemTraceStat::startKernel(kernelName);
}

void memStatWaitGroupAccess() {
  MemTraceStat::add("WAIT_GROUP", pando::getCurrentPlace().node, sizeof(int64_t));
}

} // namespace pando
