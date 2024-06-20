// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#ifndef PANDO_RT_SRC_DRVX_DRVX_HPP_
#define PANDO_RT_SRC_DRVX_DRVX_HPP_

/**
 * @defgroup DRVX DRVX
 *
 * [Drv-x] is an [SST]-based PANDO hardware simulator that compiles applications to native
 * PANDOHammer instructions and simulates the PANDO hardware in a cycle-accurate manner.
 *
 * [Drv-x]: https://github.com/AMDResearch/pando-drv "Drv-X"
 * [SST]: https://sst-simulator.org/ "SST"
 * @{
 */
/**@}*/

#include "DrvAPI.hpp"

#include "log.hpp"

namespace pando {

/**
 * @brief Translates DrvX global/static objects to native objects on a specific place.
 *
 * @ingroup DRVX
 */
template <typename U, typename T = typename U::value_type>
DrvAPI::DrvAPIPointer<T> toNativeDrvPtr(const U& globalDrvObj, Place place) {
  auto pxn = place.node.id;
  auto pod = place.pod.x;
  auto core_y = DrvAPI::coreYFromId(place.core.x);
  auto core_x = DrvAPI::coreXFromId(place.core.x);
  return DrvAPI::toGlobalAddress(&globalDrvObj, pxn, pod, core_y, core_x);
}

/**
 * @brief Translates DrvX global/static objects to ptr to native object on a specific PXN/node.
 *
 * @warning This function assumes that the object is in main memory and not in L1SP/L2SP
 *
 * @ingroup DRVX
 */
template <typename U, typename T = typename U::value_type>
DrvAPI::DrvAPIPointer<T> toNativeDrvPointerOnDram(const U& globalDrvObj, NodeIndex node) {
  DrvAPI::DrvAPIVAddress vaddr = static_cast<DrvAPI::DrvAPIAddress>(&globalDrvObj);
  if (!vaddr.is_dram()) {
    SPDLOG_ERROR("DrvX DRAM Global/Static object expected to be in main memory");
    return DrvAPI::DrvAPIAddress(0);
  }
  vaddr.pxn() = node.id;
  return vaddr.encode();
}

/**
 * @brief Yields to the next hart.
 *
 * @warning This function should be called when only one call is needed.
 *
 * @ingroup DRVX
 */
void hartYield(int cycle = 1000) noexcept;

/**
 * @brief DRVX utility class with helper types and functions to query the system configuration.
 *
 * @ingroup DRVX
 */
class Drvx {
public:
  // shorthand for l1sp
  template <typename T>
  using StaticL1SP = DrvAPI::DrvAPIGlobalL1SP<T>;

  // shorthand for l2sp
  template <typename T>
  using StaticL2SP = DrvAPI::DrvAPIGlobalL2SP<T>;

  // shorthand for main-mem
  template <typename T>
  using StaticMainMem = DrvAPI::DrvAPIGlobalDRAM<T>;

  /**
   * @brief Returns the total number of PH cores in the entire system
   */
  static std::int64_t getNumSystemCores() noexcept;

  /**
   * @brief Returns the total number of PH cores on a single PXN
   */
  static std::int64_t getNumPxnCores() noexcept;

  /**
   * @brief Returns the current node index.
   */
  static NodeIndex getCurrentNode() noexcept;

  /**
   * @brief Returns the current pod index.
   */
  static PodIndex getCurrentPod() noexcept;

  /**
   * @brief Returns the current core index.
   */
  static CoreIndex getCurrentCore() noexcept;

  /**
   * @brief Returns the node dimensions.
   */
  static NodeIndex getNodeDims() noexcept;

  /**
   * @brief Returns the pod dimensions.
   */
  static PodIndex getPodDims() noexcept;

  /**
   * @brief Returns the core dimensions.
   */
  static CoreIndex getCoreDims() noexcept;

  /**
   * @brief Returns the current thread index.
   */
  static ThreadIndex getCurrentThread() noexcept;

  /**
   * @brief Returns the thread dimensions.
   */
  static ThreadIndex getThreadDims() noexcept;

  /**
   * @brief Returns if the calling thread is on the CP.
   */
  static bool isOnCP() noexcept;
};

} // namespace pando

#endif // PANDO_RT_SRC_DRVX_DRVX_HPP_
