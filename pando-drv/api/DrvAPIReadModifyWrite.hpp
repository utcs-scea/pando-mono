// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_READ_MODIFY_WRITE_H
#define DRV_API_READ_MODIFY_WRITE_H
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <utility>
namespace DrvAPI {

typedef enum {
  DrvAPIMemAtomicCAS,
  DrvAPIMemAtomicSWAP,
  DrvAPIMemAtomicADD,
  DrvAPIMemAtomicOR,
} DrvAPIMemAtomicType;

/**
 * @brief Check if the atomic operation has an extended operand
 * @param op the atomic operation
 * @return true if the atomic operation has an extended operand
 */
inline bool DrvAPIMemAtomicTypeHasExt(DrvAPIMemAtomicType op) {
  switch (op) {
    case DrvAPIMemAtomicCAS:
      return true;
    default:
      return false;
  }
}

/**
 * @param w the write operand
 * @param r the value read from memory
 * @param op the atomic operation to perform
 * @return {value to write to memory, value to return as read}
 */
template <typename IntType>
std::pair<IntType, IntType> atomic_modify(IntType w, IntType r, DrvAPIMemAtomicType op) {
  switch (op) {
    case DrvAPIMemAtomicSWAP:
      return {w, r};
    case DrvAPIMemAtomicADD:
      return {w + r, r};
    case DrvAPIMemAtomicOR:
      return {w | r, r};
    default:
      assert(false && "Something went wrong");
  }
  return {0, 0};
}

/**
 * @return {value to write to memory, value to return as read}
 * @param w the write operand
 * @param r the value read from memory
 * @param ext the extended operand
 * @param op the atomic operation to perform
 */
template <typename IntType>
std::pair<IntType, IntType> atomic_modify(IntType w, IntType r, IntType ext,
                                          DrvAPIMemAtomicType op) {
  switch (op) {
    case DrvAPIMemAtomicCAS:
      if (r == ext) {
        return {w, r};
      } else {
        return {r, r};
      }
    default:
      assert(false && "Something went wrong");
  }
  return {0, 0};
}

template <typename IntType>
void atomic_modify(IntType* w, IntType* r, IntType* o, DrvAPIMemAtomicType op) {
  std::tie(*o, *r) = atomic_modify(*w, *r, op);
}

template <typename IntType>
void atomic_modify(IntType* w, IntType* r, IntType* ext, IntType* o, DrvAPIMemAtomicType op) {
  std::tie(*o, *r) = atomic_modify(*w, *r, *ext, op);
}

inline void atomic_modify(void* w, void* r, void* o, DrvAPIMemAtomicType op, std::size_t sz) {
  switch (sz) {
    case 1:
      atomic_modify(static_cast<std::uint8_t*>(w), static_cast<std::uint8_t*>(r),
                    static_cast<std::uint8_t*>(o), op);
      break;
    case 2:
      atomic_modify(static_cast<std::uint16_t*>(w), static_cast<std::uint16_t*>(r),
                    static_cast<std::uint16_t*>(o), op);
      break;
    case 4:
      atomic_modify(static_cast<std::uint32_t*>(w), static_cast<std::uint32_t*>(r),
                    static_cast<std::uint32_t*>(o), op);
      break;
    case 8:
      atomic_modify(static_cast<std::uint64_t*>(w), static_cast<std::uint64_t*>(r),
                    static_cast<std::uint64_t*>(o), op);
      break;
  }
}

inline void atomic_modify(void* w, void* r, void* ext, void* o, DrvAPIMemAtomicType op,
                          std::size_t sz) {
  switch (sz) {
    case 1:
      atomic_modify(static_cast<std::uint8_t*>(w), static_cast<std::uint8_t*>(r),
                    static_cast<std::uint8_t*>(ext), static_cast<std::uint8_t*>(o), op);
      break;
    case 2:
      atomic_modify(static_cast<std::uint16_t*>(w), static_cast<std::uint16_t*>(r),
                    static_cast<std::uint16_t*>(ext), static_cast<std::uint16_t*>(o), op);
      break;
    case 4:
      atomic_modify(static_cast<std::uint32_t*>(w), static_cast<std::uint32_t*>(r),
                    static_cast<std::uint32_t*>(ext), static_cast<std::uint32_t*>(o), op);
      break;
    case 8:
      atomic_modify(static_cast<std::uint64_t*>(w), static_cast<std::uint64_t*>(r),
                    static_cast<std::uint64_t*>(ext), static_cast<std::uint64_t*>(o), op);
      break;
  }
}

} // namespace DrvAPI
#endif
