// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_CORES_HPP_
#define PANDO_RT_SRC_PREP_CORES_HPP_

#include <cstddef>
#include <tuple>

#include "../queue.hpp"
#include "pando-rt/execution/task.hpp"
#include "pando-rt/index.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Cores component that models PandoHammer cores and pods.
 *
 * @ingroup PREP
 */
class Cores {
public:
  using TaskQueue = Queue<Task>;

  /**
   * @brief Flag to check if the core is active.
   */
  struct CoreActiveFlag {
    void* internalData{};
    bool operator*() const noexcept;
  };

  /**
   * @brief Initializes the cores subsystem.
   *
   * @param[in] entry entry function for the harts
   * @param[in] argc  number of arguments
   * @param[in] argv  arguments
   */
  [[nodiscard]] static Status initialize(int (*entry)(int, char**), int argc, char* argv[]);

  /**
   * @brief Finalizes the cores subsystem.
   */
  static void finalize();

  /**
   * @brief Returns the pod the current function executes on.
   */
  static PodIndex getCurrentPod() noexcept;

  /**
   * @brief Returns the pod and core the current function executes on.
   */
  static CoreIndex getCurrentCore() noexcept;

  /**
   * @brief Returns the pod and core the current function executes on.
   */
  static std::tuple<PodIndex, CoreIndex> getCurrentPodAndCore() noexcept;

  /**
   * @brief Returns the pods grid dimensions.
   */
  static PodIndex getPodDims() noexcept;

  /**
   * @brief Returns the cores grid dimensions.
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
   * @brief Returns the offset of a pointer from the L1SP base address.
   *
   * @note This function assumes that it is called from within a hart and that @p ptr is a pointer
   *       to a stack variable.
   *
   * @param[in] ptr pointer to a stack variable
   *
   * @return offset of @p ptr from the L1SP base address or a negative number if this is not called
   *         from a hart or is not an address in a hart's stack
   */
  static std::ptrdiff_t getL1SPOffset(const void* ptr) noexcept;

  /**
   * @brief Returns the native address of the address described by the tuple @p podIdx, @p coreIdx,
   *        @p offset.
   *
   * @param[in] podIdx  pod index of the L1SP
   * @param[in] coreIdx core index of the L1SP
   * @param[in] offset  offset from the base address of the L1SP
   *
   * @return the native address in the L1SP of the core or @c nullptr if it cannot be resolved
   */
  static void* getL1SPLocalAdddress(PodIndex podIdx, CoreIndex coreIdx,
                                    std::size_t offset) noexcept;

  /**
   * @brief Returns the result of the application.
   */
  static int result() noexcept;

  /**
   * @brief Returns the queue associated with place @p place.
   */
  [[nodiscard]] static TaskQueue* getTaskQueue(Place place) noexcept;

  /**
   * @brief Returns a flag to check if the core is active.
   */
  static CoreActiveFlag getCoreActiveFlag() noexcept;
};

} // namespace pando

#endif // PANDO_RT_SRC_PREP_CORES_HPP_
