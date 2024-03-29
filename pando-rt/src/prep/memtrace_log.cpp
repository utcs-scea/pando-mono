// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "memtrace_log.hpp"

#include "spdlog/fmt/bin_to_hex.h"
#include "spdlog/spdlog.h"

#include "memtrace_stat.hpp"
#include "pando-rt/memory/address_translation.hpp"

namespace pando {

namespace {
const bool isPayloadEnabled = []() -> bool {
  // always show payload in tracing for backward compatibility unless it's off
  if (auto logPayload = std::getenv("PANDO_TRACING_LOG_PAYLOAD"); logPayload != nullptr) {
    return !(std::string_view(logPayload) == "off");
  } else {
    return true;
  }
}();
}

void MemTraceLogger::log(std::string_view const op, pando::NodeIndex source, pando::NodeIndex dest,
                         std::size_t size, const void* localBuffer, GlobalAddress globalAddress) {
#if PANDO_RT_ENABLE_MEM_STAT
  // do not count ACK and REQUEST messages
  size_t ackFound = op.find("ACK");
  size_t requestFound = op.find("REQUEST");

  if (ackFound == std::string_view::npos && requestFound == std::string_view::npos) {
    MemTraceStat::add(op, source, size);
  }

#ifndef PANDO_RT_TRACE_MEM_PREP
  static_cast<void>(dest);
  static_cast<void>(localBuffer);
  static_cast<void>(globalAddress);
#endif
#endif

#ifdef PANDO_RT_TRACE_MEM_PREP
#if PANDO_RT_TRACE_MEM_PREP == 1 // INTER-PXN Memory Trace is enabled
  // When both INTER-PXN TRACE feature and PANDO_RT_ENABLE_MEM_STAT feature are both enabled
  // this function is still called for the sake of counting intra-pxn memory stats,
  // however we don't want to trace INTRA-PXN memory operations, so make sure to early return
  // when the source and dest is the same node.
  if (source == dest) {
    return;
  }
#endif
  if (globalAddress == 0) {
    if (op == "FUNC") {
      if (isPayloadEnabled) {
        spdlog::info("[MEM] {} {}-{} {} ({:n} )", op, source.id, dest.id, size,
                     spdlog::to_hex(static_cast<const std::byte*>(localBuffer),
                                    static_cast<const std::byte*>(localBuffer) + size));
      } else {
        spdlog::info("[MEM] {} {}-{} {}", op, source.id, dest.id, size);
      }
    } else {
      spdlog::info("[MEM] {} {}-{}", op, source.id, dest.id);
    }
  } else {
    std::string_view memType;

    switch (pando::extractMemoryType(globalAddress)) {
      case pando::MemoryType::Main:
        memType = "MAIN";
        break;
      case pando::MemoryType::L1SP:
        memType = "L1SP";
        break;
      case pando::MemoryType::L2SP:
        memType = "L2SP";
        break;
      default:
        memType = "Unknown";
        break;
    }

    if (isPayloadEnabled) {
      spdlog::info("[MEM] {} {} {}-{} {} {:x} ({:n} )", op, memType, source.id, dest.id, size,
                   globalAddress,
                   spdlog::to_hex(static_cast<const std::byte*>(localBuffer),
                                  static_cast<const std::byte*>(localBuffer) + size));
    } else {
      spdlog::info("[MEM] {} {} {}-{} {} {:x}", op, memType, source.id, dest.id, size,
                   globalAddress);
    }
  }
#endif
}
} // namespace pando
