// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVINTERPRETER_HPP
#define RISCVINTERPRETER_HPP
#include "RISCVHart.hpp"
#include "RISCVInstructionBase.hpp"
#include <stdexcept>

class RISCVInstruction;
class RISCVInterpreter {
public:
  RISCVInterpreter() {}

  void visit(RISCVHart& hart, RISCVInstruction& instruction) {
    instruction.accept(hart, *this);
  }

#define DEFINSTR(mnemonic, value_under_mask, mask, ...)                                 \
  virtual void visit##mnemonic(__attribute__((unused)) RISCVHart& hart,                 \
                               __attribute__((unused)) RISCVInstruction& instruction) { \
    throw std::runtime_error(#mnemonic ": Not implemented");                            \
  }
#include "InstructionTable.h"
#undef DEFINSTR
};
#endif
