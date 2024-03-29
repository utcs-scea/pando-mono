// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVINSTRUCTIONBASE_HPP
#define RISCVINSTRUCTIONBASE_HPP
#include <RISCVInstructionId.hpp>
#include <cstdint>
#include <string>
class RISCVInterpreter;

/**
 * @brief Base class for all RISCV instructions.
 */
class RISCVInstruction {
public:
  RISCVInstruction(uint32_t instruction) : instruction_(instruction) {}
  virtual ~RISCVInstruction() {}
  virtual void accept(RISCVHart& hart, RISCVInterpreter& interpreter) = 0;
  virtual const char* getMnemonic() const = 0;
  virtual RISCVInstructionId getInstructionId() const = 0;
  uint32_t rs1() const {
    return (instruction_ >> 15) & 0x1F;
  }
  uint32_t rs2() const {
    return (instruction_ >> 20) & 0x1F;
  }
  uint32_t rs3() const {
    return (instruction_ >> 27) & 0x1F;
  }
  uint32_t rd() const {
    return (instruction_ >> 7) & 0x1F;
  }
  uint32_t Iimm() const {
    return (instruction_ >> 20);
  }
  int32_t SIimm() const {
    int32_t sinstruction = instruction_;
    return (sinstruction >> 20);
  }
  int32_t Simm() const {
    int32_t sinstruction = instruction_;
    return ((sinstruction >> 25) << 5) | ((sinstruction >> 7) & 0x1F);
  }
  int32_t Bimm() const {
    int32_t sinstruction = instruction_;
    return ((sinstruction >> 31) << 12) | ((sinstruction >> 7) & 0x1) << 11 |
           ((sinstruction >> 25) & 0x3F) << 5 | ((sinstruction >> 8) & 0xF) << 1;
  }
  uint32_t Uimm() const {
    return instruction_ & 0xFFFFF000;
  }
  int32_t SUimm() const {
    return instruction_ & 0xFFFFF000;
  }
  int32_t Jimm() const {
    int32_t sinstruction = instruction_;
    return ((sinstruction >> 31) << 20) | ((sinstruction >> 21) & 0x3FF) << 1 |
           ((sinstruction >> 20) & 0x1) << 11 | ((sinstruction >> 12) & 0xFF) << 12;
  }
  uint32_t shamt() const {
    return (instruction_ >> 20) & 0x1F;
  }
  uint32_t shamt5() const {
    return shamt();
  }
  uint32_t shamt6() const {
    return (instruction_ >> 20) & 0x3F;
  }
  uint32_t instruction() const {
    return instruction_;
  }
  uint32_t instruction_;
};

#endif
