// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#ifndef DRV_API_ADDRESS_TO_NATIVE_HPP
#define DRV_API_ADDRESS_TO_NATIVE_HPP
#include <DrvAPIAddress.hpp>
#include <cstddef>
namespace DrvAPI
{
/**
 * @brief Convert a DrvAPIAddress to a native pointer
 *
 * WARNING:
 * This function will not work in multi-rank simulations.
 * This function may not work depending on the memory model used.
 * This function may not work depending on the memory controller used.
 *
 * Avoid using this function if possible. But if you need to use it, it's here.
 * But use it at your own risk, and don't expect it to work for all memory models
 * and simulation configurations.
 *
 * @param address the simulator address
 * @param native the native pointer returned from translation
 * @param the number of valid bytes starting at the native pointer
 */
void DrvAPIAddressToNative(DrvAPIAddress address, void **native, std::size_t *size);
}
#endif
