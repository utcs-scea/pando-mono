// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVDECODER_HPP
#define RISCVDECODER_HPP
#include "RISCVInstruction.hpp"
#include <iomanip>
#include <sstream>

class RISCVDecoder {
public:
  RISCVDecoder() {}
  RISCVInstruction* decode(uint32_t instruction) {
#define DEFINSTR(mnemonic, value_under_mask, mask, ...)                  \
  if ((instruction & mask) == static_cast<uint32_t>(value_under_mask)) { \
    return new mnemonic##Instruction(instruction);                       \
  }
#include "InstructionTable.h"
#undef DEFINSTR
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << instruction;
    throw std::runtime_error("Unknown instruction: " + ss.str() + "");
  }
};

#endif
