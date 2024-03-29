// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SYNC_ATOMIC_HPP_
#define PANDO_RT_SYNC_ATOMIC_HPP_

#include <atomic>
#include <cstdint>

#include "../memory/global_ptr_fwd.hpp"
#include "export.h"

namespace pando {

/**
 * @brief Atomically load an object pointed to by a pointer using a specified memory order.
 *
 * @note This operation may result in a remote store for @p value.
 *
 * @param[in]  ptr   pointer to the object to read from
 * @param[out] value pointer to space to write the value to
 * @param[in]  order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::int8_t> ptr, GlobalPtr<std::int8_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::uint8_t> ptr, GlobalPtr<std::uint8_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::int16_t> ptr, GlobalPtr<std::int16_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::uint16_t> ptr, GlobalPtr<std::uint16_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::int32_t> ptr, GlobalPtr<std::int32_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::uint32_t> ptr, GlobalPtr<std::uint32_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::int64_t> ptr, GlobalPtr<std::int64_t> value,
                                std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,GlobalPtr<std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicLoad(GlobalPtr<const std::uint64_t> ptr, GlobalPtr<std::uint64_t> value,
                                std::memory_order order);

/**
 * @brief Atomically load a value from an object pointed to by a pointer using a specified memory
 *        order.
 *
 * @param[in] ptr   pointer to the object to read from
 * @param[in] order memory order to use
 *
 * @return The value loaded from @p ptr
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::int8_t atomicLoad(GlobalPtr<const std::int8_t> ptr, std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::uint8_t atomicLoad(GlobalPtr<const std::uint8_t> ptr, std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::int16_t atomicLoad(GlobalPtr<const std::int16_t> ptr, std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::uint16_t atomicLoad(GlobalPtr<const std::uint16_t> ptr,
                                         std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::int32_t atomicLoad(GlobalPtr<const std::int32_t> ptr, std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::uint32_t atomicLoad(GlobalPtr<const std::uint32_t> ptr,
                                         std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::int64_t atomicLoad(GlobalPtr<const std::int64_t> ptr, std::memory_order order);
/// @copydoc atomicLoad(GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT std::uint64_t atomicLoad(GlobalPtr<const std::uint64_t> ptr,
                                         std::memory_order order);

/**
 * @brief Atomically store an object pointed to by a pointer using a specified memory order.
 *
 * @note This operation may result in a remote load for @p value.
 *
 * @param[out] ptr   pointer to the object to modify
 * @param[in]  value pointer to the object to read from
 * @param[in]  order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int8_t> ptr, GlobalPtr<const std::int8_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint8_t> ptr, GlobalPtr<const std::uint8_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int16_t> ptr, GlobalPtr<const std::int16_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint16_t> ptr, GlobalPtr<const std::uint16_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int32_t> ptr, GlobalPtr<const std::int32_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint32_t> ptr, GlobalPtr<const std::uint32_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int64_t> ptr, GlobalPtr<const std::int64_t> value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,GlobalPtr<const std::int8_t>,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint64_t> ptr, GlobalPtr<const std::uint64_t> value,
                                 std::memory_order order);

/**
 * @brief Atomically store a value to an object pointed to by a pointer using a specified memory
 *        order.
 *
 * @param[out] ptr   pointer to the object to modify
 * @param[in]  value value to store
 * @param[in]  order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int8_t> ptr, std::int8_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint8_t> ptr, std::uint8_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int16_t> ptr, std::int16_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint16_t> ptr, std::uint16_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int32_t> ptr, std::int32_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint32_t> ptr, std::uint32_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::int64_t> ptr, std::int64_t value,
                                 std::memory_order order);
/// @copydoc atomicStore(GlobalPtr<std::int8_t>,std::int8_t,std::memory_order)
PANDO_RT_EXPORT void atomicStore(GlobalPtr<std::uint64_t> ptr, std::uint64_t value,
                                 std::memory_order order);

/**
 * @brief Atomically compare the object pointed to by a pointer with an expected value then set the
 *        object pointed to by the pointer to the desired value if the comparison succeeded.
 *
 * @param[in] ptr      pointer to the object to modify
 * @param[in] expected pointer to the expected value stored in @p ptr
 * @param[in] desired  pointer to the value to store in @p ptr
 * @param[in] success  memory order to use when storing @p desired in @p ptr succeed
 * @param[in] failure  memory order to use when the operation fails. @p failure cannot be
 *                     `std::memory_order_release` or `std::memory_order_acq_rel` and it
 *                     cannot be stronger than @p success
 *
 * @return @c true if @p desired is written to @p ptr, otherwise, @c false
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT bool atomicCompareExchange(GlobalPtr<std::int32_t> ptr,
                                           GlobalPtr<std::int32_t> expected,
                                           GlobalPtr<const std::int32_t> desired,
                                           std::memory_order success, std::memory_order failure);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,GlobalPtr<std::int32_t>,GlobalPtr<const std::int32_t>,std::memory_order,std::memory_order)
PANDO_RT_EXPORT bool atomicCompareExchange(GlobalPtr<std::uint32_t> ptr,
                                           GlobalPtr<std::uint32_t> expected,
                                           GlobalPtr<const std::uint32_t> desired,
                                           std::memory_order success, std::memory_order failure);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,GlobalPtr<std::int32_t>,GlobalPtr<const std::int32_t>,std::memory_order,std::memory_order)
PANDO_RT_EXPORT bool atomicCompareExchange(GlobalPtr<std::int64_t> ptr,
                                           GlobalPtr<std::int64_t> expected,
                                           GlobalPtr<const std::int64_t> desired,
                                           std::memory_order success, std::memory_order failure);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,GlobalPtr<std::int32_t>,GlobalPtr<const std::int32_t>,std::memory_order,std::memory_order)
PANDO_RT_EXPORT bool atomicCompareExchange(GlobalPtr<std::uint64_t> ptr,
                                           GlobalPtr<std::uint64_t> expected,
                                           GlobalPtr<const std::uint64_t> desired,
                                           std::memory_order success, std::memory_order failure);

/**
 * @brief Atomically compare the object pointed to by a pointer with an expected value then set the
 *        object pointed to by the pointer to the desired value if the comparison succeeded.
 *
 * @note On either success or failure, the atomic operations are performed with
 *       @c std::memory_order_seq_cst memory order.
 *
 * @param[in] ptr      pointer to the object to modify
 * @param[in] expected expected value stored in @p ptr
 * @param[in] desired  value to store in @p ptr
 *
 * @return old value of @p ptr
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::int32_t atomicCompareExchange(GlobalPtr<std::int32_t> ptr,
                                                   std::int32_t expected, std::int32_t desired);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,std::int32_t,std::int32_t)
PANDO_RT_EXPORT std::uint32_t atomicCompareExchange(GlobalPtr<std::uint32_t> ptr,
                                                    std::uint32_t expected, std::uint32_t desired);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,std::int32_t,std::int32_t)
PANDO_RT_EXPORT std::int64_t atomicCompareExchange(GlobalPtr<std::int64_t> ptr,
                                                   std::int64_t expected, std::int64_t desired);
/// @copydoc atomicCompareExchange(GlobalPtr<std::int32_t>,std::int32_t,std::int32_t)
PANDO_RT_EXPORT std::uint64_t atomicCompareExchange(GlobalPtr<std::uint64_t> ptr,
                                                    std::uint64_t expected, std::uint64_t desired);

/**
 * @brief Atomically adds @p value to the value pointed to by @p ptr.
 *
 * @warning This is a fire-and-forget operation.

 * @param[inout] ptr   pointer to the object to modify
 * @param[in]    value value to add to the object pointed to by @p ptr
 * @param[in]    order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicIncrement(GlobalPtr<std::int32_t> ptr, std::int32_t value,
                                     std::memory_order order);
/// @copydoc atomicIncrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicIncrement(GlobalPtr<std::uint32_t> ptr, std::uint32_t value,
                                     std::memory_order order);
/// @copydoc atomicIncrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicIncrement(GlobalPtr<std::int64_t> ptr, std::int64_t value,
                                     std::memory_order order);
/// @copydoc atomicIncrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicIncrement(GlobalPtr<std::uint64_t> ptr, std::uint64_t value,
                                     std::memory_order order);

/**
 * @brief Atomically subtracts @p value from the value pointed to by @p ptr.
 *
 * @warning This is a fire-and-forget operation.

 * @param[inout] ptr   pointer to the object to modify
 * @param[in]    value value to subtract from the object pointed to by @p ptr
 * @param[in]    order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicDecrement(GlobalPtr<std::int32_t> ptr, std::int32_t value,
                                     std::memory_order order);
/// @copydoc atomicDecrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicDecrement(GlobalPtr<std::uint32_t> ptr, std::uint32_t value,
                                     std::memory_order order);
/// @copydoc atomicDecrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicDecrement(GlobalPtr<std::int64_t> ptr, std::int64_t value,
                                     std::memory_order order);
/// @copydoc atomicDecrement(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT void atomicDecrement(GlobalPtr<std::uint64_t> ptr, std::uint64_t value,
                                     std::memory_order order);

/**
 * @brief Atomically adds @p value to the value pointed to by @p ptr and returns the previous value
 *        held.
 *
 * @param[inout] ptr   pointer to the object to modify
 * @param[in]    value value to add to the object pointed to by @p ptr
 * @param[in]    order memory order to use
 *
 * @return value before the operation
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT std::int32_t atomicFetchAdd(GlobalPtr<std::int32_t> ptr, std::int32_t value,
                                            std::memory_order order);
/// @copydoc  atomicFetchAdd(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::uint32_t atomicFetchAdd(GlobalPtr<std::uint32_t> ptr, std::uint32_t value,
                                             std::memory_order order);
/// @copydoc  atomicFetchAdd(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::int64_t atomicFetchAdd(GlobalPtr<std::int64_t> ptr, std::int64_t value,
                                            std::memory_order order);
/// @copydoc  atomicFetchAdd(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::uint64_t atomicFetchAdd(GlobalPtr<std::uint64_t> ptr, std::uint64_t value,
                                             std::memory_order order);

/**
 * @brief Atomically subtracts @p value from the value pointed to by @p ptr and returns the previous
 *        value held.
 *
 * @param[inout] ptr   pointer to the object to modify
 * @param[in]    value value to subtract from the object pointed to by @p ptr
 * @param[in]    order memory order to use
 *
 * @return value before the operation
 *
 * @ingroup ROOT
 * */
PANDO_RT_EXPORT std::int32_t atomicFetchSub(GlobalPtr<std::int32_t> ptr, std::int32_t value,
                                            std::memory_order order);
/// @copydoc atomicFetchSub(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::uint32_t atomicFetchSub(GlobalPtr<std::uint32_t> ptr, std::uint32_t value,
                                             std::memory_order order);
/// @copydoc atomicFetchSub(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::int64_t atomicFetchSub(GlobalPtr<std::int64_t> ptr, std::int64_t value,
                                            std::memory_order order);
/// @copydoc atomicFetchSub(GlobalPtr<std::int32_t>,std::int32_t,std::memory_order)
PANDO_RT_EXPORT std::uint64_t atomicFetchSub(GlobalPtr<std::uint64_t> ptr, std::uint64_t value,
                                             std::memory_order order);

/**
 * @brief Establish a memory synchronization ordering of non-atomic and relaxed atomic
 *        operations and using the @p order semantics.
 *
 * @param[in] order memory order to use
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void atomicThreadFence(std::memory_order order);

} // namespace pando

#endif // PANDO_RT_SYNC_ATOMIC_HPP_
