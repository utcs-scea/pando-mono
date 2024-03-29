// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_STATUS_HPP_
#define PANDO_RT_STATUS_HPP_

#include <cstdint>
#include <string_view>

namespace pando {

/**
 * @brief Status codes.
 *
 * @ingroup ROOT
 */
enum class Status : std::uint32_t {
  Success = 0x0,

  Error = 0x1,
  NotImplemented = 0x2,
  InvalidValue = 0x3,

  InitError = 0x10,
  NotInit = 0x11,
  AlreadyInit = 0x12,

  MemoryError = 0x20,
  BadAlloc = 0x21,
  OutOfBounds = 0x22,
  InsufficientSpace = 0x23,

  LaunchError = 0x30,
  EnqueueError = 0x31,
};

/**
 * @brief Returns the error message associated with @p status.
 *
 * @ingroup ROOT
 */
constexpr std::string_view errorString(Status status) noexcept {
  switch (status) {
    case Status::Success:
      return "Success";
    case Status::Error:
      return "Error";
    case Status::NotImplemented:
      return "Not Implemented";
    case Status::InvalidValue:
      return "Invalid Value";
    case Status::InitError:
      return "Initialization Error";
    case Status::NotInit:
      return "Not Initialized";
    case Status::AlreadyInit:
      return "Already Initialized";
    case Status::MemoryError:
      return "Memory Error";
    case Status::BadAlloc:
      return "Allocation Error";
    case Status::OutOfBounds:
      return "Out of Bounds";
    case Status::InsufficientSpace:
      return "Insufficient Space";
    case Status::LaunchError:
      return "Launch Error";
    case Status::EnqueueError:
      return "Enqueue Error";
    default:
      return "Unknown error";
  }
}

} // namespace pando

#endif //  PANDO_RT_STATUS_HPP_
