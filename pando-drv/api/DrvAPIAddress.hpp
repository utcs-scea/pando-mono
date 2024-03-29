// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_ADDRESS_H
#define DRV_API_ADDRESS_H
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace DrvAPI {

/**
 * @brief The address type
 */
using DrvAPIAddress = uint64_t;

/**
 * @brief the types of memory
 */
typedef enum __DrvAPIMemoryType {
  DrvAPIMemoryL1SP,
  DrvAPIMemoryL2SP,
  DrvAPIMemoryDRAM,
  DrvAPIMemoryNTypes,
} DrvAPIMemoryType;

} // namespace DrvAPI
#endif
