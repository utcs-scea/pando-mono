// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#ifndef DRV_API_ADDRESS_MAP_H
#define DRV_API_ADDRESS_MAP_H
#include <DrvAPIAddress.hpp>
#include <DrvAPIInfo.hpp>
#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <utility>
namespace DrvAPI {

namespace bits {

template <typename UINT, unsigned HI, unsigned LO, unsigned TAG = 0>
struct bitrange_handle {
public:
  typedef UINT uint_type;
  static constexpr unsigned HI_BIT = HI;
  static constexpr unsigned LO_BIT = LO;

  bitrange_handle(UINT& i) : i(i) {}
  ~bitrange_handle() = default;
  bitrange_handle(bitrange_handle&& o) = default;
  bitrange_handle& operator=(bitrange_handle&& o) = default;
  bitrange_handle(const bitrange_handle& o) = default;
  bitrange_handle& operator=(const bitrange_handle& o) = default;

  static constexpr UINT lo() {
    return LO;
  }
  static constexpr UINT hi() {
    return HI;
  }
  static constexpr UINT bits() {
    return HI - LO + 1;
  }

  static constexpr UINT mask() {
    return ((1ull << (HI - LO + 1)) - 1) << LO;
  }

  static UINT getbits(UINT in) {
    return (in & mask()) >> LO;
  }

  static void setbits(UINT& in, UINT val) {
    in &= ~mask();
    in |= mask() & (val << LO);
  }

  operator UINT() const {
    return getbits(i);
  }

  bitrange_handle& operator=(UINT val) {
    setbits(i, val);
    return *this;
  }

  UINT& i;
};

} // namespace bits

struct DrvAPIPAddress; //!< forward declaration

/**
 * This is a decoded software address
 */
struct DrvAPIVAddress {
  static constexpr unsigned TAG = 0;
  typedef bits::bitrange_handle<DrvAPIAddress, 63, 63>
      CtrlRegisterHandle; //!< handle for the CtrlRegister bit
  typedef bits::bitrange_handle<DrvAPIAddress, 47, 47>
      NotScratchpadHandle; //!< handle for the NotScratchpad bit
  typedef bits::bitrange_handle<DrvAPIAddress, 46, 33> PXNHandle;    //!< handle for the PXN bits
  typedef bits::bitrange_handle<DrvAPIAddress, 32, 32> GlobalHandle; //!< handle for the global bit
  typedef bits::bitrange_handle<DrvAPIAddress, 31, 26> PodHandle;    //!< handle for the Pod bits
  typedef bits::bitrange_handle<DrvAPIAddress, 25, 25>
      L2NotL1Handle;                                                //!< handle for the L2NotL1 bit
  typedef bits::bitrange_handle<DrvAPIAddress, 22, 20> CoreYHandle; //!< handle for the CoreY bits
  typedef bits::bitrange_handle<DrvAPIAddress, 19, 17> CoreXHandle; //!< handle for the CoreX bits
  typedef bits::bitrange_handle<DrvAPIAddress, 16, 0, TAG>
      L1OffsetHandle; //!< handle for the L1Offset bits
  typedef bits::bitrange_handle<DrvAPIAddress, 24, 0, TAG>
      L2OffsetHandle; //!< handle for the L2Offset bits
  typedef bits::bitrange_handle<DrvAPIAddress, 57, 48>
      DRAMOffsetHi10Handle; //!< handle for the DRAMOffsetHi bits
  typedef bits::bitrange_handle<DrvAPIAddress, 32, 0>
      DRAMOffsetLo33Handle; //!< handle for the DRAMOffsetLo bits

  static DrvAPIPAddress to_physical(DrvAPIAddress addr, uint32_t this_pxn, uint32_t this_pod,
                                    uint32_t this_core_y, uint32_t this_core_x);

  static DrvAPIVAddress MyL2Base() {
    DrvAPIVAddress addr = 0;
    addr.ctrl_register() = false;
    addr.l2_not_l1() = true;
    addr.global() = false;
    addr.not_scratchpad() = false;
    return addr;
  }

  static DrvAPIVAddress MyL1Base() {
    DrvAPIVAddress addr = 0;
    addr.ctrl_register() = false;
    addr.l2_not_l1() = false;
    addr.global() = false;
    addr.not_scratchpad() = false;
    return addr;
  }

  static DrvAPIVAddress MainMemBase(uint32_t pxn) {
    DrvAPIVAddress addr = 0;
    addr.ctrl_register() = false;
    addr.not_scratchpad() = true;
    addr.pxn() = pxn;
    addr.global() = false;
    addr.dram_offset_hi10() = 0;
    addr.dram_offset_lo33() = 0;
    return addr;
  }

  static DrvAPIVAddress CoreCtrlBase(uint32_t pxn, uint32_t pod, uint32_t core_y, uint32_t core_x) {
    DrvAPIVAddress addr = 0;
    addr.ctrl_register() = true;
    addr.not_scratchpad() = true;
    addr.pxn() = pxn;
    addr.global() = false;
    addr.pod() = pod;
    addr.core_y() = core_y;
    addr.core_x() = core_x;
    return addr;
  }
  DrvAPIVAddress() : addr(0) {}
  DrvAPIVAddress(const DrvAPIVAddress& o) = default;
  DrvAPIVAddress& operator=(const DrvAPIVAddress& o) = default;
  DrvAPIVAddress(DrvAPIVAddress&& o) = default;
  DrvAPIVAddress& operator=(DrvAPIVAddress&& o) = default;
  ~DrvAPIVAddress() = default;

  DrvAPIVAddress(DrvAPIAddress addr) : addr(addr) {}

  CtrlRegisterHandle ctrl_register() {
    return CtrlRegisterHandle(addr);
  }

  PXNHandle pxn() {
    return PXNHandle(addr);
  }

  GlobalHandle global() {
    return GlobalHandle(addr);
  }

  PodHandle pod() {
    return PodHandle(addr);
  }

  L2NotL1Handle l2_not_l1() {
    return L2NotL1Handle(addr);
  }

  CoreYHandle core_y() {
    return CoreYHandle(addr);
  }

  CoreXHandle core_x() {
    return CoreXHandle(addr);
  }

  L1OffsetHandle l1_offset() {
    return L1OffsetHandle(addr);
  }

  L1OffsetHandle ctrl_offset() {
    return L1OffsetHandle(addr);
  }

  L2OffsetHandle l2_offset() {
    return L2OffsetHandle(addr);
  }

  NotScratchpadHandle not_scratchpad() {
    return NotScratchpadHandle(addr);
  }

  DRAMOffsetHi10Handle dram_offset_hi10() {
    return DRAMOffsetHi10Handle(addr);
  }

  DRAMOffsetLo33Handle dram_offset_lo33() {
    return DRAMOffsetLo33Handle(addr);
  }

  DrvAPIAddress dram_offset() {
    return (dram_offset_hi10() << 33) | dram_offset_lo33();
  }

  bool is_ctrl_register() {
    return ctrl_register();
  }

  bool is_dram() {
    return !is_ctrl_register() && not_scratchpad();
  }

  bool is_l2() {
    return !is_ctrl_register() && !is_dram() && l2_not_l1();
  }

  bool is_l1() {
    return !is_ctrl_register() && !is_dram() && !l2_not_l1();
  }

  DrvAPIPAddress to_physical(uint32_t this_pxn, uint32_t this_pod, uint32_t core_y,
                             uint32_t core_x);

  DrvAPIAddress encode() const {
    return addr;
  }

  std::string to_string();

  DrvAPIAddress addr;
};

/**
 * make a global address from a local address
 */
static inline DrvAPIAddress toGlobalAddress(DrvAPIAddress local, uint32_t pxn, uint32_t pod,
                                            uint32_t core_y, uint32_t core_x) {
  DrvAPIVAddress vaddr(local);
  if (vaddr.not_scratchpad()) {
    return local;
  } else if (vaddr.global()) {
    return local;
  } else if (vaddr.is_l2()) {
    vaddr.pxn() = pxn;
    vaddr.pod() = pod;
    vaddr.global() = true;
    return vaddr.encode();
  } else if (vaddr.is_l1()) {
    vaddr.pxn() = pxn;
    vaddr.pod() = pod;
    vaddr.global() = true;
    vaddr.core_y() = core_y;
    vaddr.core_x() = core_x;
    return vaddr.encode();
  }
  throw std::runtime_error("toGlobalAddress: Unknown address type");
}

static inline DrvAPIAddress toGlobalAddress(DrvAPIAddress local, uint32_t pxn, uint32_t pod,
                                            uint32_t core) {
  return toGlobalAddress(local, pxn, pod, coreYFromId(core), coreXFromId(core));
}

/**
 * This is a decoded physical address
 */
struct DrvAPIPAddress {
  static constexpr unsigned TAG = 1;
  typedef bits::bitrange_handle<DrvAPIAddress, 63, 58> TypeHandle;  //!< handle for the Type bits
  static constexpr uint32_t TYPE_L1SP = 0b000000;                   //!< L1 scratchpad
  static constexpr uint32_t TYPE_L2SP = 0b000001;                   //!< L2 scratchpad
  static constexpr uint32_t TYPE_DRAM = 0b000100;                   //!< Main memory
  static constexpr uint32_t TYPE_CTRL = 0b001000;                   //!< Control Register
  typedef bits::bitrange_handle<DrvAPIAddress, 57, 44> PXNHandle;   //!< handle for the PXN bits
  typedef bits::bitrange_handle<DrvAPIAddress, 39, 34> PodHandle;   //!< handle for the Pod bits
  typedef bits::bitrange_handle<DrvAPIAddress, 30, 28> CoreYHandle; //!< handle for the CoreY bits
  typedef bits::bitrange_handle<DrvAPIAddress, 24, 22> CoreXHandle; //!< handle for the CoreX bits
  typedef bits::bitrange_handle<DrvAPIAddress, 16, 0, TAG>
      L1OffsetHandle; //!< handle for the L1Offset bits
  typedef bits::bitrange_handle<DrvAPIAddress, 24, 0, TAG>
      L2OffsetHandle; //!< handle for the L2Offset bits
  typedef bits::bitrange_handle<DrvAPIAddress, 43, 0, TAG>
      DRAMOffsetHandle; //!< handle for the DRAMOffset bits
  typedef bits::bitrange_handle<DrvAPIAddress, 18, 18>
      CtrlIsCoreHandle; //!< handle for the CtrlIsCore bits
  typedef bits::bitrange_handle<DrvAPIAddress, 17, 0>
      CtrlOffsetHandle; //!< handle for the CtrlOffset bits

  /* core control registers */
  static constexpr uint64_t CTRL_CORE_RESET = 0x000; //!< core reset register

  DrvAPIPAddress() : addr(0) {}
  DrvAPIPAddress(const DrvAPIPAddress& o) = default;
  DrvAPIPAddress& operator=(const DrvAPIPAddress& o) = default;
  DrvAPIPAddress(DrvAPIPAddress&& o) = default;
  DrvAPIPAddress& operator=(DrvAPIPAddress&& o) = default;
  ~DrvAPIPAddress() = default;

  DrvAPIPAddress(DrvAPIAddress addr) : addr(addr) {}

  TypeHandle type() {
    return TypeHandle(addr);
  }

  PXNHandle pxn() {
    return PXNHandle(addr);
  }

  PodHandle pod() {
    return PodHandle(addr);
  }

  CoreYHandle core_y() {
    return CoreYHandle(addr);
  }

  CoreXHandle core_x() {
    return CoreXHandle(addr);
  }

  L1OffsetHandle l1_offset() {
    return L1OffsetHandle(addr);
  }

  L2OffsetHandle l2_offset() {
    return L2OffsetHandle(addr);
  }

  DRAMOffsetHandle dram_offset() {
    return DRAMOffsetHandle(addr);
  }

  CtrlIsCoreHandle ctrl_is_core() {
    return CtrlIsCoreHandle(addr);
  }

  CtrlOffsetHandle ctrl_offset() {
    return CtrlOffsetHandle(addr);
  }

  DrvAPIAddress encode() const {
    return addr;
  }

  std::string to_string();

  DrvAPIAddress addr;
};

} // namespace DrvAPI

#endif
