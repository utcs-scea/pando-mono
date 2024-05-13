// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "nodes.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

#include "gasnet.h"
#include "gasnet_coll.h"

#include "config.hpp"
#include "data_type.hpp"
#include "index.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "memtrace_log.hpp"
#include "memtrace_stat.hpp"
#include "status.hpp"

namespace pando {

namespace {

// Supported active message types
enum class AMType : std::size_t {
  GenericRequest = 0x0,
  Load,
  Store,
  AtomicLoad,
  AtomicStore,
  AtomicCompareExchange,
  AtomicIncrement,
  AtomicDecrement,
  AtomicFetchAdd,
  AtomicFetchSub,
  LoadAck,
  Ack,
  ValueAck,
  Count
};

// Convert AMType to underlying integral type
constexpr auto operator+(AMType e) noexcept {
  return static_cast<std::underlying_type_t<AMType>>(e);
}

// GASNet AM arguments are 32bit, so pointers need to be packed and unpacked for 64bit systems

// Number of arguments required for a pointer
constexpr auto ptrNArgs = sizeof(void*) / sizeof(gex_AM_Arg_t);

// Converts a pointer to hi, lo bits
std::tuple<gex_AM_Arg_t, gex_AM_Arg_t> packPtr(void* ptr) noexcept {
  static_assert(sizeof(void*) == ptrNArgs * sizeof(gex_AM_Arg_t));
  return std::make_tuple(static_cast<gex_AM_Arg_t>(GASNETI_HIWORD(ptr)),
                         static_cast<gex_AM_Arg_t>(GASNETI_LOWORD(ptr)));
}

// Converts hi, lo bits to a pointer
void* unpackPtr(gex_AM_Arg_t hi, gex_AM_Arg_t lo) noexcept {
  static_assert(sizeof(void*) == ptrNArgs * sizeof(gex_AM_Arg_t));
  return reinterpret_cast<void*>(GASNETI_MAKEWORD(hi, lo));
}

// Calculates the number of bytes required for t
template <typename... T>
constexpr std::size_t packedSize(const T&... t) noexcept {
  const auto size = (... + sizeof(t));
  return size;
}

// Packs t in buffer and returns a pointer after the packed data
template <typename... T>
void* pack(void* buffer, const T&... t) noexcept {
  auto p = static_cast<std::byte*>(buffer);
  ((std::memcpy(p, &t, sizeof(t)), p += sizeof(t)), ...);
  return p;
}

// Unpacks t from buffer and returns a pointer after the unpacked data
template <typename... T>
void* unpack(void* buffer, T&... t) noexcept {
  auto p = static_cast<std::byte*>(buffer);
  ((std::memcpy(&t, p, sizeof(t)), p += sizeof(t)), ...);
  return p;
}

#if PANDO_MEM_TRACE_OR_STAT

// Returns the source of the message associated with the token.
gasnet_node_t getMessageSource(gex_Token_t token) {
  gasnet_node_t source;
  gasnet_AMGetMsgSource(token, &source);
  return source;
}

#endif

// Processes a generic request
void handleRequest(gex_Token_t, void*, size_t);
// Processes a load request
void handleLoad(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo);
// Processes a store request
void handleStore(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo);
// Processes an atomic load request
void handleAtomicLoad(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                      gex_AM_Arg_t handlePtrLo);
// Processes an atomic load request
void handleAtomicStore(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                       gex_AM_Arg_t handlePtrLo);
// Processes an atomic compare-exchange request
void handleAtomicCompareExchange(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                                 gex_AM_Arg_t handlePtrLo);
// Processes an atomic increment request
void handleAtomicInc(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                     gex_AM_Arg_t handlePtrLo);
// Processes an atomic decrement request
void handleAtomicDec(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                     gex_AM_Arg_t handlePtrLo);
// Processes an atomic fetch-add request
void handleAtomicFetchAdd(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                          gex_AM_Arg_t handlePtrLo);
// Processes an atomic fetch-sub request
void handleAtomicFetchSub(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi,
                          gex_AM_Arg_t handlePtrLo);
// Processes an ack for a load
void handleLoadAck(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo);
// Processes an ack
void handleAck(gex_Token_t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo);
// Processes an ack with a value
void handleValueAck(gex_Token_t, void*, size_t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo);

// System wide GASNet client
struct {
  std::string_view clientName = "pando-rt";
  std::int64_t rank{GEX_RANK_INVALID};
  std::int64_t size{GEX_RANK_INVALID};
  gex_Client_t client{GEX_CLIENT_INVALID};
  gex_EP_t endpoint{GEX_EP_INVALID};
  gex_TM_t team{GEX_TM_INVALID};
  std::atomic<bool> pollingThreadActive{true};
  std::thread pollingThread;

  gex_AM_Entry_t htable[+AMType::Count] = {
      // generic request
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleRequest), (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM),
       0, nullptr, nullptr},

      // load / store
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleLoad), (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM),
       ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleStore), (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM),
       ptrNArgs, nullptr, nullptr},

      // atomics
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicLoad),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicStore),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicCompareExchange),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicInc),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicDec),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicFetchAdd),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAtomicFetchSub),
       (GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_MEDIUM), ptrNArgs, nullptr, nullptr},

      // acks
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleLoadAck), (GEX_FLAG_AM_REQREP | GEX_FLAG_AM_MEDIUM),
       ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleAck), (GEX_FLAG_AM_REQREP | GEX_FLAG_AM_SHORT),
       ptrNArgs, nullptr, nullptr},
      {0, reinterpret_cast<gex_AM_Fn_t>(&handleValueAck), (GEX_FLAG_AM_REQREP | GEX_FLAG_AM_MEDIUM),
       ptrNArgs, nullptr, nullptr},
  };
} world;

// Sends an ack
void sendAck(gex_Token_t token, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  const auto flags = 0;
  if (auto status = gex_AM_ReplyShort(token, world.htable[+AMType::Ack].gex_index, flags,
                                      handlePtrHi, handlePtrLo);
      status != GASNET_OK) {
    SPDLOG_ERROR("Could not send ack: {} ({})", gasnet_ErrorDesc(status), gasnet_ErrorName(status));
    std::abort();
  }
}

// Sends a value
template <typename IntType>
void sendValue(gex_Token_t token, IntType t, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  static_assert(std::is_integral_v<IntType>);

  const auto flags = 0;
  if (auto status =
          gex_AM_ReplyMedium(token, world.htable[+AMType::ValueAck].gex_index, &t, sizeof(IntType),
                             GEX_EVENT_NOW, flags, handlePtrHi, handlePtrLo);
      status != GASNET_OK) {
    SPDLOG_ERROR("Could not send value: {} ({})", gasnet_ErrorDesc(status),
                 gasnet_ErrorName(status));
    std::abort();
  }
}

// Processes a generic request (i.e., RPC)
void handleRequest(gex_Token_t token, void* buffer, size_t byteCount) {
  auto* request = static_cast<detail::Request*>(buffer);
  if (auto status = (*request)(); status != Status::Success) {
    SPDLOG_ERROR("Failed to execute remote operation: {}", status);
    std::abort();
  }

#if PANDO_MEM_TRACE_OR_STAT
  MemTraceLogger::log("FUNC", NodeIndex(getMessageSource(token)), NodeIndex(world.rank), byteCount,
                      buffer);
#else
  static_cast<void>(token);
  static_cast<void>(byteCount);
#endif
}

// Processes a load
void handleLoad(gex_Token_t token, void* buffer, size_t /*byteCount*/, gex_AM_Arg_t handlePtrHi,
                gex_AM_Arg_t handlePtrLo) {
  // unpack
  GlobalAddress srcAddr;
  std::size_t n;
  unpack(buffer, srcAddr, n);

  // send reply message with data
  void* srcDataPtr = Memory::getNativeAddress(srcAddr);
  const auto flags = 0;
  if (auto status = gex_AM_ReplyMedium(token, world.htable[+AMType::LoadAck].gex_index, srcDataPtr,
                                       n, GEX_EVENT_NOW, flags, handlePtrHi, handlePtrLo);
      status != GASNET_OK) {
    SPDLOG_ERROR("Could not send value: {} ({})", gasnet_ErrorDesc(status),
                 gasnet_ErrorName(status));

    std::abort();
  }

#if PANDO_MEM_TRACE_OR_STAT
  MemTraceLogger::log("LOAD", NodeIndex(getMessageSource(token)), NodeIndex(world.rank), n,
                      srcDataPtr, srcAddr);
#endif
}

// Processes a store
void handleStore(gex_Token_t token, void* buffer, size_t byteCount, gex_AM_Arg_t handlePtrHi,
                 gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from total byte count
  GlobalAddress dstAddr;
  const void* srcDataPtr = unpack(buffer, dstAddr);
  const auto n = byteCount - packedSize(dstAddr);

  // write data payload to global address
  void* nativeDstPtr = Memory::getNativeAddress(dstAddr);
  std::memcpy(nativeDstPtr, srcDataPtr, n);
  // TODO(ypapadop-amd): remove when remote atomics are used by user applications
  std::atomic_thread_fence(std::memory_order_release);

  sendAck(token, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
  MemTraceLogger::log("STORE", NodeIndex(getMessageSource(token)), NodeIndex(world.rank), n,
                      nativeDstPtr, dstAddr);
#endif
}

// Processes an atomic load for an integral type
struct AtomicLoadImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress srcAddr, gex_AM_Arg_t handlePtrHi,
                  gex_AM_Arg_t handlePtrLo) {
    auto srcNativePtr = static_cast<const IntType*>(Memory::getNativeAddress(srcAddr));
    IntType retValue = __atomic_load_n(srcNativePtr, __ATOMIC_RELAXED);
    sendValue(token, retValue, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_LOAD", NodeIndex(getMessageSource(token)), NodeIndex(world.rank),
                        sizeof(retValue), &retValue, srcAddr);
#endif
  }
};

// Processes an atomic load
void handleAtomicLoad(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                      gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack
  GlobalAddress srcAddr;
  DataType dataType;
  unpack(buffer, srcAddr, dataType);

  dataTypeDispatch(dataType, AtomicLoadImpl{}, token, srcAddr, handlePtrHi, handlePtrLo);
}

// Processes an atomic store for an integral type
struct AtomicStoreImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, const void* data,
                  gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto srcPtr = static_cast<const IntType*>(data);
    __atomic_store(dstNativePtr, srcPtr, __ATOMIC_RELAXED);
    sendAck(token, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_STORE", NodeIndex(getMessageSource(token)), NodeIndex(world.rank),
                        sizeof(IntType), dstNativePtr, dstAddr);
#endif
  }
};

// Processes an atomic store
void handleAtomicStore(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                       gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  const void* srcDataPtr = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicStoreImpl{}, token, dstAddr, srcDataPtr, handlePtrHi,
                   handlePtrLo);
}

// Processes an atomic compare-exchange for an integral type
struct AtomicCompareExchangeImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, void* data, gex_AM_Arg_t handlePtrHi,
                  gex_AM_Arg_t handlePtrLo) {
    constexpr bool weak = false;
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto expectedPtr = static_cast<IntType*>(data);
    const IntType* desiredPtr = expectedPtr + 1;
    __atomic_compare_exchange(dstNativePtr, expectedPtr, desiredPtr, weak, __ATOMIC_RELAXED,
                              __ATOMIC_RELAXED);
    sendValue(token, *expectedPtr, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_COMPARE_EXCHANGE", NodeIndex(getMessageSource(token)),
                        NodeIndex(world.rank), sizeof(IntType), dstNativePtr, dstAddr);
#endif
  }
};

// Processes an atomic compare-exchange
void handleAtomicCompareExchange(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                                 gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  void* srcData = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicCompareExchangeImpl{}, token, dstAddr, srcData, handlePtrHi,
                   handlePtrLo);
}

// Processes an atomic increment for an integral type
struct AtomicIncImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, const void* data,
                  gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto valuePtr = static_cast<const IntType*>(data);
    __atomic_fetch_add(dstNativePtr, *valuePtr, __ATOMIC_RELAXED);
    sendAck(token, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_INCREMENT", NodeIndex(getMessageSource(token)),
                        NodeIndex(world.rank), sizeof(IntType), dstNativePtr, dstAddr);
#endif
  }
};

// Processes an atomic increment
void handleAtomicInc(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                     gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  const void* srcDataPtr = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicIncImpl{}, token, dstAddr, srcDataPtr, handlePtrHi, handlePtrLo);
}

// Processes an atomic decrement for an integral type
struct AtomicDecImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, const void* data,
                  gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto valuePtr = static_cast<const IntType*>(data);
    __atomic_fetch_sub(dstNativePtr, *valuePtr, __ATOMIC_RELAXED);
    sendAck(token, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_DECREMENT", NodeIndex(getMessageSource(token)),
                        NodeIndex(world.rank), sizeof(IntType), dstNativePtr, dstAddr);
#endif
  }
};

// Processes an atomic decrement
void handleAtomicDec(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                     gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  const void* srcDataPtr = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicDecImpl{}, token, dstAddr, srcDataPtr, handlePtrHi, handlePtrLo);
}

// Processes an atomic fetch-add for an integral type
struct AtomicFetchAddImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, const void* data,
                  gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto valuePtr = static_cast<const IntType*>(data);
    IntType retValue = __atomic_fetch_add(dstNativePtr, *valuePtr, __ATOMIC_RELAXED);
    sendValue(token, retValue, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_FETCH_ADD", NodeIndex(getMessageSource(token)),
                        NodeIndex(world.rank), sizeof(retValue), &retValue, dstAddr);
#endif
  }
};

// Processes an atomic fetch-add
void handleAtomicFetchAdd(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                          gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  const void* srcDataPtr = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicFetchAddImpl{}, token, dstAddr, srcDataPtr, handlePtrHi,
                   handlePtrLo);
}

// Processes an atomic fetch-sub for an integral type
struct AtomicFetchSubImpl {
  template <typename IntType>
  void operator()(gex_Token_t token, GlobalAddress dstAddr, const void* data,
                  gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
    auto dstNativePtr = static_cast<IntType*>(Memory::getNativeAddress(dstAddr));
    auto valuePtr = static_cast<const IntType*>(data);
    IntType retValue = __atomic_fetch_sub(dstNativePtr, *valuePtr, __ATOMIC_RELAXED);
    sendValue(token, retValue, handlePtrHi, handlePtrLo);

#if PANDO_MEM_TRACE_OR_STAT
    MemTraceLogger::log("ATOMIC_FETCH_SUB", NodeIndex(getMessageSource(token)),
                        NodeIndex(world.rank), sizeof(retValue), &retValue, dstAddr);
#endif
  }
};

// Processes an atomic fetch-sub
void handleAtomicFetchSub(gex_Token_t token, void* buffer, size_t /*byteCount*/,
                          gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  // unpack: payload number of bytes inferred from data type
  GlobalAddress dstAddr;
  DataType dataType;
  const void* srcDataPtr = unpack(buffer, dstAddr, dataType);

  dataTypeDispatch(dataType, AtomicFetchSubImpl{}, token, dstAddr, srcDataPtr, handlePtrHi,
                   handlePtrLo);
}

// Processes an ack for a load
void handleLoadAck(gex_Token_t token, void* buffer, size_t byteCount, gex_AM_Arg_t handlePtrHi,
                   gex_AM_Arg_t handlePtrLo) {
  auto handlePtr = reinterpret_cast<Nodes::LoadHandle*>(unpackPtr(handlePtrHi, handlePtrLo));
  handlePtr->setReady(buffer, byteCount);

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("LOAD_ACK", NodeIndex(world.rank), NodeIndex(getMessageSource(token)));
#else
  static_cast<void>(token);
#endif
}

// Processes an ack. This is just a signal with no payload.
void handleAck(gex_Token_t token, gex_AM_Arg_t handlePtrHi, gex_AM_Arg_t handlePtrLo) {
  auto handlePtr = reinterpret_cast<Nodes::AckHandle*>(unpackPtr(handlePtrHi, handlePtrLo));
  handlePtr->setReady();

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ACK", NodeIndex(world.rank), NodeIndex(getMessageSource(token)));
#else
  static_cast<void>(token);
#endif
}

// Processes an ack with a value
void handleValueAck(gex_Token_t token, void* buffer, size_t /*byteCount*/, gex_AM_Arg_t handlePtrHi,
                    gex_AM_Arg_t handlePtrLo) {
  auto handlePtr = reinterpret_cast<Nodes::ValueHandleBase*>(unpackPtr(handlePtrHi, handlePtrLo));
  handlePtr->setReady(buffer);

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("VALUE_ACK", NodeIndex(world.rank), NodeIndex(getMessageSource(token)));
#else
  static_cast<void>(token);
#endif
}

// GASNet polling function
void processMessages(std::atomic<bool>& pollingActive) {
  // block the thread until stopped, while polling GASNet
  while (pollingActive.load(std::memory_order_relaxed) == true) {
    GASNET_BLOCKUNTIL(pollingActive.load(std::memory_order_relaxed) == false);
  }
  // TODO(ashwin): Below code does polling instead of blocking, which may be useful if we
  // switch the CP to run on a qthread/hart.
  // do {
  //   gasnet_AMPoll();
  // } while (pollingActive.load(std::memory_order_relaxed) == true);
}

} // namespace

Status Nodes::initialize() {
  const auto& config = Config::getCurrentConfig();

  // initialize library
  if (auto status = gex_Client_Init(&world.client, &world.endpoint, &world.team,
                                    world.clientName.data(), nullptr, nullptr, 0);
      status != GASNET_OK) {
    SPDLOG_ERROR("Error initializing GASNet: {} ({})", gasnet_ErrorDesc(status),
                 gasnet_ErrorName(status));
    return Status::Error;
  }

  if (auto maxThreads = gex_System_QueryMaxThreads(); maxThreads < (config.compute.coreCount + 1)) {
    SPDLOG_ERROR(
        "GASNet supports up to {} threads per process, but {} worker + 1 polling threads were "
        "requested.\nReduce the number of threads, e.g., by reducing PANDO_PREP_NUM_CORES.",
        maxThreads, config.compute.coreCount);
  }

  world.rank = gex_TM_QueryRank(world.team);
  world.size = gex_TM_QuerySize(world.team);

  // initialize AM
  if (auto status = gex_EP_RegisterHandlers(world.endpoint, world.htable,
                                            sizeof(world.htable) / sizeof(gex_AM_Entry_t));
      status != GASNET_OK) {
    SPDLOG_ERROR("Node {} - Error initializing GASNet AM: {} ({})", world.rank,
                 gasnet_ErrorDesc(status), gasnet_ErrorName(status));
    return Status::Error;
  }

  // start polling thread
  world.pollingThread = std::thread(processMessages, std::ref(world.pollingThreadActive));

  // wait for all nodes to finish initialization
  auto barrierEvent = gex_Coll_BarrierNB(world.team, 0);
  gex_Event_Wait(barrierEvent);

  SPDLOG_INFO("Node {} - GASNet initialized", Nodes::getCurrentNode());

  return Status::Success;
}

void Nodes::finalize() {
  // stop and wait for polling thread
  world.pollingThreadActive.store(false, std::memory_order_relaxed);
  world.pollingThread.join();

  gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
}

void Nodes::exit(int errorCode) {
  SPDLOG_WARN("Terminating with code {}", errorCode);

  // stop polling thread and exit
  world.pollingThreadActive.store(false, std::memory_order_relaxed);
  gasnet_exit(errorCode);
}

NodeIndex Nodes::getCurrentNode() noexcept {
  return NodeIndex(world.rank);
}

NodeIndex Nodes::getNodeDims() noexcept {
  return NodeIndex(world.size);
}

Status Nodes::requestAcquire(NodeIndex nodeIdx, std::size_t requestSize, void** p,
                             void** metadata) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 0;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // write output variables
  *p = buffer;
  *metadata = sd;
  return Status::Success;
}

void Nodes::requestRelease(std::size_t requestSize, void* metadata) {
  auto sd = static_cast<gex_AM_SrcDesc_t>(metadata);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium(sd, world.htable[+AMType::GenericRequest].gex_index, requestSize);
}

Status Nodes::load(NodeIndex nodeIdx, GlobalAddress srcAddr, std::size_t n, LoadHandle& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  // size payload
  const auto requestSize = packedSize(srcAddr, n);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, srcAddr, n);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::Load].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("LOAD_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

Status Nodes::store(NodeIndex nodeIdx, GlobalAddress dstAddr, std::size_t n, const void* srcPtr,
                    AckHandle& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  // size payload: number of bytes to write is inferred from byteCount
  const auto requestSize = packedSize(dstAddr) + n;

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload: number of bytes to write is inferred from total byte count
  auto packedDataEnd = pack(buffer, dstAddr);
  std::memcpy(packedDataEnd, srcPtr, n);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::Store].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("STORE_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicLoad(NodeIndex nodeIdx, GlobalAddress srcAddr, ValueHandle<T>& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(srcAddr, dataType);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, srcAddr, dataType);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicLoad].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#if PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_LOAD_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicStore(NodeIndex nodeIdx, GlobalAddress dstAddr, T value, AckHandle& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, value);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, value);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicStore].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_STORE_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicCompareExchange(NodeIndex nodeIdx, GlobalAddress dstAddr, T expected, T desired,
                                    ValueHandle<T>& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, expected, desired);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, expected, desired);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicCompareExchange].gex_index,
                              requestSize, std::get<0>(packedHandlePtr),
                              std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_COMPARE_EXCHANGE_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicIncrement(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                              AckHandle& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, value);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, value);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicIncrement].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_INCREMENT_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicDecrement(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                              AckHandle& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, value);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, value);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicDecrement].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_DECREMENT_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicFetchAdd(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                             ValueHandle<T>& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, value);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, value);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicFetchAdd].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_FETCH_ADD_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

template <typename T>
Status Nodes::atomicFetchSub(NodeIndex nodeIdx, GlobalAddress dstAddr, T value,
                             ValueHandle<T>& handle) {
  if (nodeIdx >= getNodeDims()) {
    SPDLOG_ERROR("Node index out of bounds: {}", nodeIdx);
    return Status::OutOfBounds;
  }

  constexpr auto dataType = DataTypeTraits<T>::dataType;

  // size payload
  const auto requestSize = packedSize(dstAddr, dataType, value);

  // get managed buffer to write the request in
  const gex_Flags_t flags = 0;
  const unsigned int numArgs = 2;
  const auto maxMediumRequest =
      gex_AM_MaxRequestMedium(world.team, nodeIdx.id, nullptr, flags, numArgs);
  if (requestSize > maxMediumRequest) {
    SPDLOG_ERROR("Request too large: {} > {}", requestSize, maxMediumRequest);
    return Status::BadAlloc;
  }
  gex_AM_SrcDesc_t sd = gex_AM_PrepareRequestMedium(world.team, nodeIdx.id, nullptr, requestSize,
                                                    requestSize, GEX_EVENT_NOW, flags, numArgs);
  auto buffer = gex_AM_SrcDescAddr(sd);
  if (buffer == nullptr) {
    SPDLOG_ERROR("Could not allocate space to send to node {}", nodeIdx);
    return Status::BadAlloc;
  }

  // pack payload
  pack(buffer, dstAddr, dataType, value);
  // pack pointer for reply
  auto packedHandlePtr = packPtr(&handle);
  // mark buffer ready for send
  gex_AM_CommitRequestMedium2(sd, world.htable[+AMType::AtomicFetchSub].gex_index, requestSize,
                              std::get<0>(packedHandlePtr), std::get<1>(packedHandlePtr));

#ifdef PANDO_RT_TRACE_MEM_PREP
  MemTraceLogger::log("ATOMIC_FETCH_SUB_REQUEST", getCurrentNode(), nodeIdx);
#endif

  return Status::Success;
}

void Nodes::barrier() {
  const gex_Flags_t flags = 0;
  gex_Event_Wait(gex_Coll_BarrierNB(world.team, flags));
}

// TODO(ashwin): Below code does poll-test + thread yield instead of block waiting, which may be
// useful if we switch the CP to run on a qthread/hart.
// // Poll for completion of non-blocking GASNet operations
// void eventPollWait(gex_Event_t event) {
//   do {
//     gasnet_AMPoll();
//     hartYield();
//   } while (gex_Event_Test(event) != GASNET_OK);
// }

std::int64_t Nodes::allreduce(std::int64_t value) {
  const gex_Flags_t flags = 0;
  gex_Event_Wait(gex_Coll_ReduceToAllNB(world.team, &value, &value, GEX_DT_I64, sizeof(value), 1,
                                        GEX_OP_ADD, nullptr, nullptr, flags));
  return value;
}

// explicit template instantiations based on supported types

#define INSTANTIATE_ATOMIC_LOAD_STORE(T)                                           \
  template Status Nodes::atomicLoad<T>(NodeIndex, GlobalAddress, ValueHandle<T>&); \
  template Status Nodes::atomicStore<T>(NodeIndex, GlobalAddress, T, AckHandle&);
INSTANTIATE_ATOMIC_LOAD_STORE(std::int8_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::uint8_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::int16_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::uint16_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::int32_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::uint32_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::int64_t)
INSTANTIATE_ATOMIC_LOAD_STORE(std::uint64_t)
#undef INSTANTIATE_ATOMIC_LOAD_STORE

#define INSTANTIATE_ATOMICS(T)                                                            \
  template Status Nodes::atomicCompareExchange<T>(NodeIndex, GlobalAddress, T, T,         \
                                                  ValueHandle<T>&);                       \
  template Status Nodes::atomicIncrement<T>(NodeIndex, GlobalAddress, T, AckHandle&);     \
  template Status Nodes::atomicDecrement<T>(NodeIndex, GlobalAddress, T, AckHandle&);     \
  template Status Nodes::atomicFetchAdd<T>(NodeIndex, GlobalAddress, T, ValueHandle<T>&); \
  template Status Nodes::atomicFetchSub<T>(NodeIndex, GlobalAddress, T, ValueHandle<T>&);
INSTANTIATE_ATOMICS(std::int32_t)
INSTANTIATE_ATOMICS(std::uint32_t)
INSTANTIATE_ATOMICS(std::int64_t)
INSTANTIATE_ATOMICS(std::uint64_t)
#undef INSTANTIATE_ATOMICS

} // namespace pando
