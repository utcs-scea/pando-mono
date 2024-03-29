// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/memory/memory_info.hpp"

#include <memory>

#include "pando-rt/locality.hpp"
#include "pando-rt/memory/address_map.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "specific_storage.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/config.hpp"
#include "prep/memory.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "pando-rt/stdlib.hpp"

#include "drvx/drvx.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

std::size_t getThreadStackSize() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  const auto& config = Config::getCurrentConfig();
  return config.memory.l1SPHart;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return DrvAPI::coreL1SPSize() / DrvAPI::numCoreThreads();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

std::size_t getThreadAvailableStack() noexcept {
  if (isOnCP()) {
    return 0;
  }
  const auto& thisPlace = getCurrentPlace();
  auto stackBegin = globalPtrReinterpretCast<GlobalPtr<std::byte>>(encodeL1SPAddress(
      thisPlace.node, thisPlace.pod, thisPlace.core, getCurrentThread().id * getThreadStackSize()));
  GlobalPtr<std::byte> ptr = reinterpret_cast<std::byte*>(std::addressof(ptr));
  return ptr - stackBegin;
}

std::size_t getNodeL2SPSize() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  const auto& config = Config::getCurrentConfig();
  return config.memory.l2SPPod;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return DrvAPI::podL2SPSize() * DrvAPI::numPXNPods();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

std::size_t getNodeMainMemorySize() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  const auto& config = Config::getCurrentConfig();
  return config.memory.mainNode;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return DrvAPI::pxnDRAMSize();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

namespace detail {

std::pair<GlobalPtr<std::byte>, std::size_t> getMemoryStartAndSize(MemoryType memoryType) noexcept {
  const auto reservedByteCount = getReservedMemorySpace(memoryType);

#if defined(PANDO_RT_USE_BACKEND_PREP)

  const auto& memoryInformation = Memory::getInformation(memoryType);
  return {GlobalPtr<std::byte>(memoryInformation.baseAddress) + reservedByteCount,
          memoryInformation.byteCount - reservedByteCount};

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  DrvAPI::DrvAPIMemoryType sectionType;
  std::size_t memoryBytes{};

  auto place = getCurrentPlace();

  switch (memoryType) {
    case MemoryType::Main: {
      sectionType = DrvAPI::DrvAPIMemoryType::DrvAPIMemoryDRAM;
      memoryBytes = getNodeMainMemorySize();
    } break;
    case MemoryType::L2SP: {
      sectionType = DrvAPI::DrvAPIMemoryType::DrvAPIMemoryL2SP;
      memoryBytes = getNodeL2SPSize();

      // The L2SP is segmented in the DrvX address map, however, in PREP's address map it is not
      // segmented.
      const auto placeDims = getPlaceDims();
      if (placeDims.pod.x != 1 && placeDims.pod.x != 1) {
        PANDO_ABORT("Only PODs of dimensions [x, y] = [1, 1] are supported for the DrvX backend.");
      }

      // we will only initialize POD 0, 0
      place.pod.x = 0;
      place.pod.y = 0;
    } break;
    default: {
      PANDO_ABORT(
          "Only MainMemory and L2SP for the DrvX backend are supported for dynamic allocations.");
    }
  }

  const auto& section = DrvAPI::DrvAPISection::GetSection(sectionType);
  auto localBase = section.getBase(static_cast<std::uint32_t>(place.node.id),
                                   static_cast<std::uint32_t>(place.pod.x),
                                   static_cast<std::uint32_t>(place.core.x));

  auto globalBase = DrvAPI::toGlobalAddress(
      localBase, static_cast<std::uint32_t>(place.node.id), static_cast<std::uint32_t>(place.pod.x),
      static_cast<std::uint32_t>(place.core.y), static_cast<std::uint32_t>(place.core.x));

  auto baseAddress = globalPtrReinterpretCast<GlobalPtr<std::byte>>(globalBase) + reservedByteCount;
  auto byteCount = memoryBytes - reservedByteCount;

  return {baseAddress, byteCount};

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace detail

} // namespace pando
