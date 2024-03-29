// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_MEMORY_HPP_
#define PANDO_RT_SRC_PREP_MEMORY_HPP_

#include <cstddef>

#include "pando-rt/memory/global_ptr_fwd.hpp"
#include "pando-rt/memory/memory_type.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Memory component models the per-PXN memories.
 *
 * @ingroup PREP
 */
class Memory {
public:
  /**
   * @brief Memory information.
   */
  struct Information {
    /// @brief Memory type
    MemoryType type{MemoryType::Unknown};
    /// @brief Memory base address
    std::byte* baseAddress{};
    /// @brief Memory size in bytes
    std::size_t byteCount{};
  };

  /**
   * @brief Initializes the memory subsystem.
   *
   * @param[in] l2SPZeroFillBytes number of bytes in L2SP memory to set to zero
   * @param[in] mainZeroFillBytes number of bytes in main memory to set to zero
   */
  [[nodiscard]] static Status initialize(std::size_t l2SPZeroFillBytes,
                                         std::size_t mainZeroFillBytes);

  /**
   * @brief Finalizes the cores subsystem.
   */
  static void finalize();

  /**
   * @brief Returns the memory information that corresponds to @p memoryType.
   *
   * @param[in] memoryType memory type
   *
   * @return the requested memory information or an empty object if @ref MemoryType::L1SP
   *         was requested
   */
  static const Information& getInformation(MemoryType memoryType) noexcept;

  /**
   * @brief Finds the memory @p nativePtr points to.
   *
   * @note It is advised that pointers are accompanied by their memory information if possible
   *       instead of using this function.
   *
   * @warning @p nativePtr must point to a known memory otherwise this function will result
   *          in undefined behavior.
   *
   * @param[in] nativePtr pointer to a known memory type
   *
   * @return memory information of @p nativePtr or @ref MemoryType::Unknown if it could not be found
   */
  static const Information& findInformation(const void* nativePtr) noexcept;

  /**
   * @brief Returns a native address corresponding to a global address when it is resolvable in this
   *        node.
   *
   * @param[in] addr global address
   *
   * @return native address corresponding to @p addr or @c nullptr if the address is not from this
   *         node
   */
  static void* getNativeAddress(GlobalAddress addr) noexcept;
};

} // namespace pando

#endif // PANDO_RT_SRC_PREP_MEMORY_HPP_
