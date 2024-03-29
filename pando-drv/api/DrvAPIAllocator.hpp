// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_ALLOCATOR_H
#define DRV_API_ALLOCATOR_H

#include <DrvAPIMemory.hpp>
#include <DrvAPIPointer.hpp>

namespace DrvAPI {

/**
 * @brief Allocate memory
 *
 * @param type
 * @param size
 * @return DrvAPIPointer<uint8_t>
 */
DrvAPIPointer<void> DrvAPIMemoryAlloc(DrvAPIMemoryType type, size_t size);

/**
 * @brief Allocate memory
 *
 * @param type
 * @param size
 * @return DrvAPIPointer<uint8_t>
 */
void DrvAPIMemoryFree(const DrvAPIPointer<void>& ptr);

/**
 * @brief Initialize the memory allocator
 *
 */
void DrvAPIMemoryAllocatorInit();

} // namespace DrvAPI
#endif
