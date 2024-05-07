// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_ADDRESS_TRANSLATION_HPP_
#define PANDO_RT_MEMORY_ADDRESS_TRANSLATION_HPP_

#include <cstdint>
#include <type_traits>

#include "../index.hpp"
#include "../utility/bit_manip.hpp"
#include "address_map.hpp"
#include "global_ptr_fwd.hpp"
#include "memory_type.hpp"
#if defined(PANDO_RT_USE_BACKEND_DRVX)
#include "DrvAPIAddressMap.hpp"
#endif // PANDO_RT_USE_BACKEND_DRVX

namespace pando {

/**
 * @brief Extracts the memory type of the memory from global address @p addr.
 *
 * @param[in] addr global address
 *
 * @return memory type @p addr references
 *
 * @ingroup ROOT
 */
inline MemoryType extractMemoryType(GlobalAddress addr) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  return static_cast<MemoryType>(readBits(addr, addressMap.memoryType));
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::DrvAPIVAddress vaddr = addr;
  if (vaddr.is_dram()) {
    return MemoryType::Main;
  } else if (vaddr.is_l2()) {
    return MemoryType::L2SP;
  } else if (vaddr.is_l1()) {
    return MemoryType::L1SP;
  } else {
    return MemoryType::Unknown;
  }
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Extracts the node index from global address @p addr.
 *
 * @warning This is only valid for @ref MemoryType::L1SP, @ref MemoryType::L2SP and
 *          @ref MemoryType::Main memories.
 *
 * @param[in] addr global address
 *
 * @return node index @p addr references
 *
 * @ingroup ROOT
 */
inline NodeIndex extractNodeIndex(GlobalAddress addr) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  return NodeIndex{std::int64_t(readBits(addr, addressMap.main.nodeIndex))};
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::DrvAPIVAddress vaddr = addr;
  return NodeIndex(vaddr.pxn());
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Extracts the pod index from global address @p addr.
 *
 * @warning This is only valid for @ref MemoryType::L1SP and @ref MemoryType::L2SP memories.
 *
 * @param[in] addr global address
 *
 * @return pod index @p addr references
 *
 * @ingroup ROOT
 */
inline PodIndex extractPodIndex(GlobalAddress addr) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  return PodIndex{std::int8_t(readBits(addr, addressMap.L1SP.podX)),
                  std::int8_t(readBits(addr, addressMap.L1SP.podY))};
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::DrvAPIVAddress vaddr = addr;
  return PodIndex(vaddr.pod(), 0);
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Extracts the core index from global address @p addr.
 *
 * @warning This is only valid for @ref MemoryType::L1SP memory.
 *
 * @param[in] addr global address
 *
 * @return core index @p addr references
 *
 * @ingroup ROOT
 */
inline CoreIndex extractCoreIndex(GlobalAddress addr) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  return CoreIndex{std::int8_t(readBits(addr, addressMap.L1SP.coreX)),
                   std::int8_t(readBits(addr, addressMap.L1SP.coreY))};
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::DrvAPIVAddress vaddr = addr;
  return CoreIndex(vaddr.core_x(), vaddr.core_y());
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Returns if the global bit is set for global address @p addr.
 *
 * @warning This is only valid for @ref MemoryType::L1SP memory.
 *
 * @param[in] addr global address
 *
 * @return @c true if the global visible bit is set, otherwise @c false
 *
 * @ingroup ROOT
 */
inline bool extractL1SPGlobalBit(GlobalAddress addr) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)
  return readBits(addr, addressMap.L1SP.global);
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
  DrvAPI::DrvAPIVAddress vaddr = addr;
  return vaddr.global() ? true : false;
#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Converts a L1SP memory address to a global address.
 *
 * @param[in] nodeIdx node index
 * @param[in] podIdx  pod index
 * @param[in] coreIdx core index
 * @param[in] offset  offset from start of L1SP base address
 *
 * @return the L1SP memory address described by the tuple
 *         <tt>(nodeIdx, podIdx, coreIdx, offset)</tt> as a global address
 *
 * @ingroup ROOT
 */
inline GlobalAddress encodeL1SPAddress(NodeIndex nodeIdx, PodIndex podIdx, CoreIndex coreIdx,
                                       std::size_t offset) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  using UIntType = std::uintptr_t;

  UIntType p = 0x0;
  p |= createMask<UIntType>(addressMap.memoryType, +MemoryType::L1SP);
  p |= createMask<UIntType>(addressMap.L1SP.nodeIndex, nodeIdx.id);
  p |= createMask<UIntType>(addressMap.L1SP.podX, podIdx.x);
  p |= createMask<UIntType>(addressMap.L1SP.podY, podIdx.y);
  p |= createMask<UIntType>(addressMap.L1SP.coreX, coreIdx.x);
  p |= createMask<UIntType>(addressMap.L1SP.coreY, coreIdx.y);
  p |= createMask<UIntType>(addressMap.L1SP.global, true);
  p |= createMask<UIntType>(addressMap.L1SP.offset, offset);
  return reinterpret_cast<GlobalAddress>(p);

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  DrvAPI::DrvAPIVAddress addr = 0;
  addr.l2_not_l1() = false;
  addr.not_scratchpad() = false;
  addr.pxn() = nodeIdx.id;
  addr.pod() = podIdx.x;
  addr.core_x() = coreIdx.x;
  addr.core_y() = coreIdx.y;
  addr.global() = true;
  addr.l1_offset() = offset;
  return addr.encode();

#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Converts a L2SP memory address to a global address.
 *
 * @param[in] nodeIdx node index
 * @param[in] podIdx  pod index
 * @param[in] offset  offset from start of L2SP base address
 *
 * @return the L1SP memory address described by the tuple <tt>(nodeIdx, podIdx, offset)</tt> as a
 *         global address
 *
 * @ingroup ROOT
 */
inline GlobalAddress encodeL2SPAddress(NodeIndex nodeIdx, PodIndex podIdx,
                                       std::size_t offset) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  using UIntType = std::uintptr_t;

  UIntType p = 0x0;
  p |= createMask<UIntType>(addressMap.memoryType, +MemoryType::L2SP);
  p |= createMask<UIntType>(addressMap.L2SP.nodeIndex, nodeIdx.id);
  p |= createMask<UIntType>(addressMap.L2SP.podX, podIdx.x);
  p |= createMask<UIntType>(addressMap.L2SP.podY, podIdx.y);
  p |= createMask<UIntType>(addressMap.L2SP.offset, offset);
  return reinterpret_cast<GlobalAddress>(p);

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  DrvAPI::DrvAPIVAddress addr = 0;
  addr.l2_not_l1() = true;
  addr.not_scratchpad() = false;
  addr.pxn() = nodeIdx.id;
  addr.pod() = podIdx.x;
  addr.l2_offset() = offset;
  addr.global() = true;
  return addr.encode();

#endif // PANDO_RT_USE_BACKEND_PREP
}

/**
 * @brief Converts a main memory address to a global address.
 *
 * @param[in] nodeIdx node index
 * @param[in] offset  offset from start of main memory base address
 *
 * @return the main memory address described by the tuple <tt>(nodeIdx, offset)</tt> as a global
 *         address
 *
 * @ingroup ROOT
 */
inline GlobalAddress encodeMainAddress(NodeIndex nodeIdx, std::size_t offset) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  using UIntType = std::uintptr_t;

  UIntType p = 0x0;
  p |= createMask<UIntType>(addressMap.memoryType, +MemoryType::Main);
  p |= createMask<UIntType>(addressMap.main.nodeIndex, nodeIdx.id);
  p |= createMask<UIntType>(addressMap.main.offset, offset);
  return reinterpret_cast<GlobalAddress>(p);

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  DrvAPI::DrvAPIVAddress addr = 0;
  addr.not_scratchpad() = true;
  addr.pxn() = nodeIdx.id;
  // separately set lo33 and hi10 offsets
  addr.dram_offset_lo33() = DrvAPI::DrvAPIVAddress::DRAMOffsetLo33Handle::getbits(offset);
  addr.dram_offset_hi10() = DrvAPI::DrvAPIVAddress::DRAMOffsetHi10Handle::getbits(offset);
  return addr.encode();

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace pando

#endif // PANDO_RT_MEMORY_ADDRESS_TRANSLATION_HPP_
