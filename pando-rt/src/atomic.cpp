// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/* Copyright (c) 2023 University of Washington */

#include "pando-rt/sync/atomic.hpp"

#include "pando-rt/memory/global_ptr.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "pando-rt/stdlib.hpp"
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#include "prep/log.hpp"
#include "prep/memory.hpp"
#include "prep/nodes.hpp"
#include "prep/status.hpp"
#if PANDO_MEM_TRACE_OR_STAT
#include "prep/memtrace_log.hpp"
#endif
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/drvx.hpp"
#include "pando-rt/drv_info.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

// Translates C++ std::memory_order to the libc equivalent
constexpr int stdToGccMemOrder(std::memory_order order) noexcept {
  return static_cast<int>(order);
}

#ifdef PANDO_RT_USE_BACKEND_PREP

// Adds a fence before the atomic operation if needed
void preAtomicOpFence(std::memory_order order) noexcept {
  switch (order) {
    case std::memory_order_release:
    case std::memory_order_acq_rel:
      std::atomic_thread_fence(std::memory_order_release);
      break;
    case std::memory_order_seq_cst:
      std::atomic_thread_fence(std::memory_order_seq_cst);
      break;
    default:
      break;
  }
}

// Adds a fence after the atomic operation if needed
void postAtomicOpFence(std::memory_order order) noexcept {
  switch (order) {
    case std::memory_order_acquire:
    case std::memory_order_acq_rel:
      std::atomic_thread_fence(std::memory_order_acquire);
      break;
    case std::memory_order_seq_cst:
      std::atomic_thread_fence(std::memory_order_seq_cst);
      break;
    default:
      break;
  }
}

#endif // PANDO_RT_USE_BACKEND_PREP

template <typename T>
T atomicLoadImpl(GlobalPtr<const T> ptr, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local load

    // do atomic load, yield to other hart, and return loaded value
    auto srcNativePtr = static_cast<const T*>(Memory::getNativeAddress(ptr.address));
    auto value = __atomic_load_n(srcNativePtr, stdToGccMemOrder(order));
    hartYield();

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_LOAD", nodeIdx, nodeIdx, sizeof(T), srcNativePtr, ptr.address);
#endif

    return value;
  } else {
    // remote load

    // 1. conditional seq_cst fence to guarantee ordering at the caller
    if (order == std::memory_order_seq_cst) {
      std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    // 2. relaxed load
    Nodes::ValueHandle<T> handle;
    if (auto status = Nodes::atomicLoad<T>(nodeIdx, ptr.address, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence to guarantee ordering at the caller
    std::atomic_thread_fence(order);

    return handle.value();
  }

#else

  return *ptr;

#endif
}

#define INSTANTIATE_ATOMIC_LOAD(T)                                                       \
  T atomicLoad(GlobalPtr<const T> ptr, std::memory_order order) {                        \
    return atomicLoadImpl(ptr, order);                                                   \
  }

INSTANTIATE_ATOMIC_LOAD(std::int8_t)
INSTANTIATE_ATOMIC_LOAD(std::uint8_t)
INSTANTIATE_ATOMIC_LOAD(std::int16_t)
INSTANTIATE_ATOMIC_LOAD(std::uint16_t)
INSTANTIATE_ATOMIC_LOAD(std::int32_t)
INSTANTIATE_ATOMIC_LOAD(std::uint32_t)
INSTANTIATE_ATOMIC_LOAD(std::int64_t)
INSTANTIATE_ATOMIC_LOAD(std::uint64_t)

#undef INSTANTIATE_ATOMIC_LOAD

template <typename T>
void atomicStoreImpl(GlobalPtr<T> ptr, const T value, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local store

    // yield to other hart and do atomic store
    //hartYield();
    auto dstNativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    __atomic_store_n(dstNativePtr, value, stdToGccMemOrder(order));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_STORE", nodeIdx, nodeIdx, sizeof(T), dstNativePtr, ptr.address);
#endif
  } else {
    // remote store

    // 1. fence to guarantee ordering at the caller
    std::atomic_thread_fence(order);

    // 2. blocking relaxed store
    Nodes::AckHandle handle;
    if (auto status = Nodes::atomicStore<T>(nodeIdx, ptr.address, value, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. conditional seq_cst fence to guarantee ordering at the caller
    if (order == std::memory_order_seq_cst) {
      std::atomic_thread_fence(std::memory_order_seq_cst);
    }
  }

#else

  *ptr = value;

#endif
}

#define INSTANTIATE_ATOMIC_STORE(T)                                                       \
  void atomicStore(GlobalPtr<T> ptr, T value, std::memory_order order) {                  \
    atomicStoreImpl(ptr, value, order);                                                   \
  }

INSTANTIATE_ATOMIC_STORE(std::int8_t)
INSTANTIATE_ATOMIC_STORE(std::uint8_t)
INSTANTIATE_ATOMIC_STORE(std::int16_t)
INSTANTIATE_ATOMIC_STORE(std::uint16_t)
INSTANTIATE_ATOMIC_STORE(std::int32_t)
INSTANTIATE_ATOMIC_STORE(std::uint32_t)
INSTANTIATE_ATOMIC_STORE(std::int64_t)
INSTANTIATE_ATOMIC_STORE(std::uint64_t)

#undef INSTANTIATE_ATOMIC_STORE

template <typename T>
bool atomicCompareExchangeImpl(GlobalPtr<T> ptr, T& expected, const T desired,
                                   [[maybe_unused]] std::memory_order success,
                                   [[maybe_unused]] std::memory_order failure) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local compare-exchange

    constexpr bool weak = false;

    // do compare-exchange, yield has already happened
    auto nativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    auto result = __atomic_compare_exchange_n(nativePtr, &expected, desired, weak,
                                              stdToGccMemOrder(success), stdToGccMemOrder(failure));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_COMPARE_EXCHANGE", nodeIdx, nodeIdx, sizeof(T), nativePtr,
                        ptr.address);
#endif

    return result;
  } else {
    // remote compare-exchange

    // 1. fence to guarantee ordering at the caller; check only for success, as it should be
    // stronger that failure
    preAtomicOpFence(success);

    // 2. relaxed atomic compare-exchange
    Nodes::ValueHandle<T> handle;
    if (auto status = Nodes::atomicCompareExchange<T>(nodeIdx, ptr.address, expected,
                                                      desired, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence upon success / failure
    if (handle.value() == expected) {
      // success
      postAtomicOpFence(success);
      return true;
    } else {
      // failure
      postAtomicOpFence(failure);
      expected = handle.value();
      return false;
    }
  }

#else

#if defined(PANDO_RT_BYPASS_INIT)
  T foundValue;
  if (DrvAPI::isStageInit()) {
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(ptr.address, &addr_native, &size);
    T* as_native_pointer = reinterpret_cast<T*>(addr_native);
    __atomic_compare_exchange_n(as_native_pointer, &expected, desired, false, static_cast<int>(std::memory_order_relaxed), static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);

    foundValue = expected;
  } else {
    foundValue = DrvAPI::atomic_cas<T>(ptr.address, expected, desired);
  }
#else
  const auto foundValue = DrvAPI::atomic_cas<T>(ptr.address, expected, desired);
#endif
  if (foundValue == expected) {
    return true;
  }
  else {
    expected = foundValue;
    return false;
  }

#endif
}

template <typename T>
bool atomicCompareExchangeImpl(GlobalPtr<T> ptr, T& expected, const T desired) {
  return atomicCompareExchangeImpl<T>(ptr, expected, desired, std::memory_order_seq_cst,
                                          std::memory_order_seq_cst);
}

#define INSTANTIATE_ATOMIC_COMPARE_EXCHANGE(T)                                                    \
  bool atomicCompareExchange(GlobalPtr<T> ptr, T& expected, const T desired,                  \
                                 std::memory_order success, std::memory_order failure) {          \
    return atomicCompareExchangeImpl(ptr, expected, desired, success, failure);               \
  }                                                                                               \
  bool atomicCompareExchange(GlobalPtr<T> ptr, T& expected, const T desired) {                \
    return atomicCompareExchangeImpl(ptr, expected, desired);                                 \
  }

INSTANTIATE_ATOMIC_COMPARE_EXCHANGE(std::int32_t)
INSTANTIATE_ATOMIC_COMPARE_EXCHANGE(std::int64_t)
INSTANTIATE_ATOMIC_COMPARE_EXCHANGE(std::uint32_t)
INSTANTIATE_ATOMIC_COMPARE_EXCHANGE(std::uint64_t)

#undef INSTANTIATE_ATOMIC_COMPARE_EXCHANGE

template <typename T>
void atomicIncrementImpl(GlobalPtr<T> ptr, T value, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local increment

    // yield to other hart and do atomic increment
    //hartYield();
    auto nativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    __atomic_fetch_add(nativePtr, value, stdToGccMemOrder(order));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_INCREMENT", nodeIdx, nodeIdx, sizeof(T), nativePtr, ptr.address);
#endif
  } else {
    // remote increment

    // 1. fence to guarantee ordering at the caller
    preAtomicOpFence(order);

    // 2. blocking relaxed atomic increment
    Nodes::AckHandle handle;
    if (auto status = Nodes::atomicIncrement<T>(nodeIdx, ptr.address, value, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence to guarantee ordering at the caller
    postAtomicOpFence(order);
  }

#else

#if defined(PANDO_RT_BYPASS_INIT)
  if (DrvAPI::isStageInit()) {
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(ptr.address, &addr_native, &size);
    T* as_native_pointer = reinterpret_cast<T*>(addr_native);
    __atomic_fetch_add(as_native_pointer, value, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
  } else {
    DrvAPI::atomic_add(ptr.address, value);
  }
#else
  DrvAPI::atomic_add(ptr.address, value);
#endif

#endif
}

#define INSTANTIATE_ATOMIC_INCREMENT(T)                                      \
  void atomicIncrement(GlobalPtr<T> ptr, T value, std::memory_order order) { \
    return atomicIncrementImpl(ptr, value, order);                           \
  }

INSTANTIATE_ATOMIC_INCREMENT(std::int32_t)
INSTANTIATE_ATOMIC_INCREMENT(std::int64_t)
INSTANTIATE_ATOMIC_INCREMENT(std::uint32_t)
INSTANTIATE_ATOMIC_INCREMENT(std::uint64_t)

#undef INSTANTIATE_ATOMIC_INCREMENT

template <typename T>
void atomicDecrementImpl(GlobalPtr<T> ptr, T value, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local increment

    // yield to other hart and do atomic decrement
    //hartYield();
    auto nativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    __atomic_fetch_sub(nativePtr, value, stdToGccMemOrder(order));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_DECREMENT", nodeIdx, nodeIdx, sizeof(T), nativePtr, ptr.address);
#endif
  } else {
    // remote decrement

    // 1. fence to guarantee ordering at the caller
    preAtomicOpFence(order);

    // 2. blocking relaxed atomic decrement
    Nodes::AckHandle handle;
    if (auto status = Nodes::atomicDecrement<T>(nodeIdx, ptr.address, value, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence to guarantee ordering at the caller
    postAtomicOpFence(order);
  }

#else

#if defined(PANDO_RT_BYPASS_INIT)
  if (DrvAPI::isStageInit()) {
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(ptr.address, &addr_native, &size);
    T* as_native_pointer = reinterpret_cast<T*>(addr_native);
    __atomic_fetch_add(as_native_pointer, static_cast<T>(-1) * value, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
  } else {
    DrvAPI::atomic_add(ptr.address, static_cast<T>(-1) * value);
  }
#else
  DrvAPI::atomic_add(ptr.address, static_cast<T>(-1) * value);
#endif

#endif
}

#define INSTANTIATE_ATOMIC_DECREMENT(T)                                      \
  void atomicDecrement(GlobalPtr<T> ptr, T value, std::memory_order order) { \
    return atomicDecrementImpl(ptr, value, order);                           \
  }

INSTANTIATE_ATOMIC_DECREMENT(std::int32_t)
INSTANTIATE_ATOMIC_DECREMENT(std::int64_t)
INSTANTIATE_ATOMIC_DECREMENT(std::uint32_t)
INSTANTIATE_ATOMIC_DECREMENT(std::uint64_t)

#undef INSTANTIATE_ATOMIC_DECREMENT

template <typename T>
T atomicFetchAddImpl(GlobalPtr<T> ptr, T value, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local increment

    // yield to other hart and do atomic fetch-add
    //hartYield();
    auto nativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    auto result = __atomic_fetch_add(nativePtr, value, stdToGccMemOrder(order));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_FETCH_ADD", nodeIdx, nodeIdx, sizeof(T), nativePtr, ptr.address);
#endif
    return result;
  } else {
    // remote increment

    // 1. fence to guarantee ordering at the caller
    preAtomicOpFence(order);

    // 2. blocking relaxed atomic fetch-add
    Nodes::ValueHandle<T> handle;
    if (auto status = Nodes::atomicFetchAdd<T>(nodeIdx, ptr.address, value, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence to guarantee ordering at the caller
    postAtomicOpFence(order);

    return handle.value();
  }

#else

#if defined(PANDO_RT_BYPASS_INIT)
  if (DrvAPI::isStageInit()) {
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(ptr.address, &addr_native, &size);
    T* as_native_pointer = reinterpret_cast<T*>(addr_native);
    auto result = __atomic_fetch_add(as_native_pointer, value, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
    return result;
  } else {
    return DrvAPI::atomic_add(ptr.address, value);
  }
#else
  return DrvAPI::atomic_add(ptr.address, value);
#endif

#endif
}

#define INSTANTIATE_ATOMIC_FETCH_ADD(T)                                  \
  T atomicFetchAdd(GlobalPtr<T> ptr, T value, std::memory_order order) { \
    return atomicFetchAddImpl(ptr, value, order);                        \
  }

INSTANTIATE_ATOMIC_FETCH_ADD(std::int32_t)
INSTANTIATE_ATOMIC_FETCH_ADD(std::int64_t)
INSTANTIATE_ATOMIC_FETCH_ADD(std::uint32_t)
INSTANTIATE_ATOMIC_FETCH_ADD(std::uint64_t)

#undef INSTANTIATE_ATOMIC_FETCH_ADD

template <typename T>
T atomicFetchSubImpl(GlobalPtr<T> ptr, T value, [[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  const auto nodeIdx = extractNodeIndex(ptr.address);
  if (nodeIdx == Nodes::getCurrentNode()) {
    // local increment

    // yield to other hart and do atomic fetch-sub
    //hartYield();
    auto nativePtr = static_cast<T*>(Memory::getNativeAddress(ptr.address));
    auto result = __atomic_fetch_sub(nativePtr, value, stdToGccMemOrder(order));

#if PANDO_MEM_TRACE_OR_STAT
    // if the level of mem-tracing is ALL (2), log intra-pxn memory operations
    MemTraceLogger::log("ATOMIC_FETCH_SUB", nodeIdx, nodeIdx, sizeof(T), nativePtr, ptr.address);
#endif

    return result;
  } else {
    // remote increment

    // 1. fence to guarantee ordering at the caller
    preAtomicOpFence(order);

    // 2. blocking relaxed atomic fetch-sub
    Nodes::ValueHandle<T> handle;
    if (auto status = Nodes::atomicFetchSub<T>(nodeIdx, ptr.address, value, handle);
        status != Status::Success) {
      SPDLOG_ERROR("Remote operation error: {}", status);
      PANDO_ABORT("Remote operation error");
    }
    hartYieldUntil([&handle] {
      return handle.ready();
    });

    // 3. fence to guarantee ordering at the caller
    postAtomicOpFence(order);

    return handle.value();
  }

#else

#if defined(PANDO_RT_BYPASS_INIT)
  if (DrvAPI::isStageInit()) {
    void *addr_native = nullptr;
    std::size_t size = 0;
    DrvAPI::DrvAPIAddressToNative(ptr.address, &addr_native, &size);
    T* as_native_pointer = reinterpret_cast<T*>(addr_native);
    auto result = __atomic_fetch_add(as_native_pointer, static_cast<T>(-1) * value, static_cast<int>(std::memory_order_relaxed));
    // hartYield
    DrvAPI::nop(1u);
    return result;
  } else {
    return DrvAPI::atomic_add(ptr.address, static_cast<T>(-1) * value);
  }
#else
  return DrvAPI::atomic_add(ptr.address, static_cast<T>(-1) * value);
#endif

#endif
}

#define INSTANTIATE_ATOMIC_FETCH_SUB(T)                                  \
  T atomicFetchSub(GlobalPtr<T> ptr, T value, std::memory_order order) { \
    return atomicFetchSubImpl(ptr, value, order);                        \
  }

INSTANTIATE_ATOMIC_FETCH_SUB(std::int32_t)
INSTANTIATE_ATOMIC_FETCH_SUB(std::int64_t)
INSTANTIATE_ATOMIC_FETCH_SUB(std::uint32_t)
INSTANTIATE_ATOMIC_FETCH_SUB(std::uint64_t)

#undef INSTANTIATE_ATOMIC_FETCH_SUB

void atomicThreadFence([[maybe_unused]] std::memory_order order) {
#ifdef PANDO_RT_USE_BACKEND_PREP

  // yield to other hart and issue fence
  //hartYield();
  std::atomic_thread_fence(order);

#else

  // TODO(max/ashwin): DrvX fence integration

#endif
}

} // namespace pando
