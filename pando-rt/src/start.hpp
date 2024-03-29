// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_START_HPP_
#define PANDO_RT_SRC_START_HPP_

/**
 * @brief Start function for each hart.
 *
 * @ingroup ROOT
 */
extern "C" int __start(int argc, char** argv);

#endif // PANDO_RT_SRC_START_HPP_
