// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_PANDO_RT_HPP_
#define PANDO_RT_PANDO_RT_HPP_

/**
 * @defgroup ROOT ROOT
 *
 * The PANDO Runtime System (pando-rt) offers ROOT, the Application Programming Interfaces (APIs) to
 * develop applications for PANDO hardware and its emulators.
 * @{
 */
/**@}*/

#include <cstdint>
#include <cstdio>

#include "execution/bulk_execute_on.hpp"
#include "execution/execute_on.hpp"
#include "execution/execute_on_wait.hpp"
#include "export.h"
#include "index.hpp"
#include "locality.hpp"
#include "main.hpp"
#include "memory/allocate_memory.hpp"
#include "memory/memory_info.hpp"
#include "memory_resource.hpp"
#include "specific_storage.hpp"
#include "status.hpp"
#include "stdlib.hpp"
#include "sync/notification.hpp"
#include "sync/wait.hpp"
#include "utility/check.hpp"

#endif // PANDO_RT_PANDO_RT_HPP_
