// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_NODES_HPP_
#define PANDO_RT_SRC_PREP_NODES_HPP_

#include <atomic>
#include <cstddef>
#include <cstring>
#include <functional>

#include "pando-rt/execution/request.hpp"
#include "pando-rt/index.hpp"
#include "pando-rt/memory/global_ptr_fwd.hpp"
#include "pando-rt/status.hpp"

namespace pando {

/**
 * @brief Nodes component that models interactions across PXNs.
 *
 * @ingroup PREP
 */
class Nodes {
public:
  /**
   * @brief Handle to retrieve result of load operations.
   */
  class LoadHandle {
    std::atomic<bool> m_done{false};
    void* m_ptr{};

  public:
    /**
     * @brief Creates a handle to wait for the operation to finish.
     *
     * @param[in] ptr pointer to write the data in
     */
    explicit LoadHandle(void* ptr) noexcept : m_ptr{ptr} {}

    LoadHandle(const LoadHandle&) = delete;
    LoadHandle(LoadHandle&&) = delete;

    ~LoadHandle() = default;

    LoadHandle& operator=(const LoadHandle&) = delete;
    LoadHandle& operator=(LoadHandle&&) = delete;

    /**
     * @brief Returns if the operation has finished.
     */
    bool ready() const noexcept {
      // checks if the load has finished; setReady() could be called from a different thread
      if (!m_done.load(std::memory_order_relaxed)) {
        return false;
      }
      std::atomic_thread_fence(std::memory_order_acquire);
      return true;
    }

    /**
     * @brief Writes the data to the provided space and marks the operation as finished.
     */
    void setReady(void* ptr, std::size_t n) noexcept {
      // write may happen from a different thread than the waiting one
      std::memcpy(m_ptr, ptr, n);
      m_done.store(true, std::memory_order_release);
    }
  };

  /**
   * @brief Handle to notify about completion of operations.
   */
  class AckHandle {
    std::atomic<bool> m_done{false};

  public:
    AckHandle() noexcept = default;

    AckHandle(const AckHandle&) = delete;
    AckHandle(AckHandle&&) = delete;

    ~AckHandle() = default;

    AckHandle& operator=(const AckHandle&) = delete;
    AckHandle& operator=(AckHandle&&) = delete;

    /**
     * @brief Returns if the operation has finished.
     */
    bool ready() const noexcept {
      // checks if the operation has finished, imposes no ordering
      return m_done.load(std::memory_order_relaxed);
    }

    /**
     * @brief Marks the operation as finished.
     */
    void setReady() noexcept {
      // signals that the operation has finished, imposes no ordering
      m_done.store(true, std::memory_order_relaxed);
    }
  };

  /**
   * @brief Base class from @ref ValueHandle.
   */
  class ValueHandleBase {
  protected:
    ~ValueHandleBase() = default;

  public:
    /**
     * @brief Writes the data to the provided space and marks the operation as finished.
     */
    virtual void setReady(const void* data) noexcept = 0;
  };

  /**
   * @brief Handle to retrieve result of operation with known result type.
   */
  template <typename T>
  class ValueHandle final : public ValueHandleBase {
    static_assert(std::is_trivially_destructible_v<T>);

    alignas(T) std::byte m_storage[sizeof(T)];
    std::atomic<bool> m_done{false};

  public:
    ValueHandle() noexcept = default;

    ValueHandle(const ValueHandle&) = delete;
    ValueHandle(ValueHandle&&) = delete;

    ~ValueHandle() = default;

    ValueHandle& operator=(const ValueHandle&) = delete;
    ValueHandle& operator=(ValueHandle&&) = delete;

    /**
     * @brief Returns if the operation has finished.
     */
    bool ready() const noexcept {
      // checks if the load has finished; setReady() could be called from a different thread
      if (!m_done.load(std::memory_order_relaxed)) {
        return false;
      }
      std::atomic_thread_fence(std::memory_order_acquire);
      return true;
    }

    /// @copydoc ValueHandleBase::setReady(const void*)
    void setReady(const void* data) noexcept override {
      *reinterpret_cast<T*>(m_storage) = *static_cast<const T*>(data);
      m_done.store(true, std::memory_order_release);
    }

    /**
     * @brief Returns the value stored in this handle.
     */
    const T& value() const {
      return *reinterpret_cast<const T*>(m_storage);
    }
  };

  /**
   * @brief Initializes the nodes subsystem.
   */
  [[nodiscard]] static Status initialize();

  /**
   * @brief Finalizes the nodes subsystem.
   */
  static void finalize();

  /**
   * @brief Terminates program execution with code @p errorCode.
   *
   * @param[in] errorCode error code to terminate with
   */
  [[noreturn]] static void exit(int errorCode);

  /**
   * @brief Returns the current node index.
   */
  static NodeIndex getCurrentNode() noexcept;

  /**
   * @brief Returns the nodes grid dimensions.
   */
  static NodeIndex getNodeDims() noexcept;

  /**
   * @brief Allocates space for a request to node @p nodeIdx.
   *
   * @param[in]  nodeIdx     destination node
   * @param[in]  requestSize request size
   * @param[out] p           pointer to store the allocated space
   * @param[out] metadata    pointer to store pointer to internal metadata for the request
   */
  [[nodiscard]] static Status requestAcquire(NodeIndex nodeIdx, std::size_t requestSize, void** p,
                                             void** metadata);

  /**
   * @brief Release the space for a request.
   *
   * @param[in] requestSize request size
   * @param[in] metadata    pointer to internal metadata for the request
   */
  static void requestRelease(std::size_t requestSize, void* metadata);

  /**
   * @brief Performs a remote load operation.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  srcAddr global address to read from
   * @param[in]  n       size in bytes to read
   * @param[out] handle  handle to notify of success
   */
  [[nodiscard]] static Status load(NodeIndex nodeIdx, GlobalAddress srcAddr, std::size_t n,
                                   LoadHandle& handle);

  /**
   * @brief Performs a remote store operation.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address to write to
   * @param[in]  n       size in bytes to write
   * @param[in]  srcPtr  native pointer to read data from
   * @param[out] handle  handle to notify of success
   */
  [[nodiscard]] static Status store(NodeIndex nodeIdx, GlobalAddress dstAddr, std::size_t n,
                                    const void* srcPtr, AckHandle& handle);

  /**
   * @brief Performs a remote atomic load operation.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  srcAddr global address to read from
   * @param[out] handle  handle to retrieve the result of the operation
   */
  template <typename T>
  [[nodiscard]] static Status atomicLoad(NodeIndex nodeIdx, GlobalAddress srcAddr,
                                         ValueHandle<T>& handle);

  /**
   * @brief Performs a remote atomic store operation.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address to write to
   * @param[in]  value   value to write
   * @param[out] handle  handle to notify of success
   */
  template <typename T>
  [[nodiscard]] static Status atomicStore(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                                          AckHandle& handle);

  /**
   * @brief Performs a remote atomic compare-exchange.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx  destination node
   * @param[in]  dstAddr  global address of object to test and modify
   * @param[in]  expected value expected to be found in @p dstAddr
   * @param[in]  desired  value to store in @p dstAddr if it is @p expected
   * @param[out] handle   handle to retrieve the result of the operation
   */
  template <typename T>
  [[nodiscard]] static Status atomicCompareExchange(NodeIndex nodeIdx, GlobalAddress dstAddr,
                                                    T expected, T desired, ValueHandle<T>& handle);

  /**
   * @brief Performs a remote atomic increment.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address of object to increment
   * @param[in]  value   value to increment by
   * @param[out] handle  handle to notify of success
   */
  template <typename T>
  [[nodiscard]] static Status atomicIncrement(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                                              AckHandle& handle);

  /**
   * @brief Performs a remote atomic decrement.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address of object to decrement
   * @param[in]  value   value to decrement by
   * @param[out] handle  handle to notify of success
   */
  template <typename T>
  [[nodiscard]] static Status atomicDecrement(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                                              AckHandle& handle);

  /**
   * @brief Performs a remote atomic fetch-add.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address of object to add to
   * @param[in]  value   value to add
   * @param[out] handle  handle to retrieve the result of the operation
   */
  template <typename T>
  [[nodiscard]] static Status atomicFetchAdd(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                                             ValueHandle<T>& handle);

  /**
   * @brief Performs a remote atomic fetch-sub.
   *
   * @note The operation imposes no ordering at the destination node.
   *
   * @param[in]  nodeIdx destination node
   * @param[in]  dstAddr global address of object to subtract from
   * @param[in]  value   value to subtract
   * @param[out] handle  handle to retrieve the result of the operation
   */
  template <typename T>
  [[nodiscard]] static Status atomicFetchSub(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                                             ValueHandle<T>& handle);

  /**
   * @brief Waits until all nodes reached the barrier.
   *
   * @note This is a collective operation across all nodes.
   */
  static void barrier();

  /**
   * @brief Performs an allreduce operation.
   *
   * @note This is a collective operation across all nodes.
   *
   * @param[in] value value to contribute to the allreduce
   */
  static std::int64_t allreduce(std::int64_t value);
};

} // namespace pando

#endif // PANDO_RT_SRC_PREP_NODES_HPP_
