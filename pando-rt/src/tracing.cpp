// SPDX-License-Identifier: MIT
/* Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/tracing.hpp"

#include "prep/memtrace_stat.hpp"

namespace pando {

void memStatNewPhase() {
  MemTraceStat::writePhase();
}

void memStatNewKernel(std::string_view kernelName) {
  MemTraceStat::startKernel(kernelName);
}

} // namespace pando