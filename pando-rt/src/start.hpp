// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_START_HPP_
#define PANDO_RT_SRC_START_HPP_

#include <cstdint>

constexpr bool IDLE_TIMER_ENABLE = false;

constexpr std::uint64_t STEAL_THRESH_HOLD_SIZE = 16;

enum SchedulerFailState{
  YIELD,
  STEAL,
};

/**
 * @brief Start function for each hart.
 *
 * @ingroup ROOT
 */
extern "C" int __start(int argc, char** argv);

#endif // PANDO_RT_SRC_START_HPP_
