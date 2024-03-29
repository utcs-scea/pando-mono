// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVINSTRUCTION_HPP
#define RISCVINSTRUCTION_HPP
#include "RISCVHart.hpp"
#include "RISCVInstructionBase.hpp"
#include "RISCVInterpreter.hpp"
#include <cstdint>

#define DEFINSTR(mnemonic, value_under_mask, mask...)                   \
    class mnemonic##Instruction : public RISCVInstruction {             \
    public:                                                             \
    static constexpr uint32_t VALUE = value_under_mask;                 \
    static constexpr uint32_t MASK = mask;                              \
    static constexpr RISCVInstructionId ID = mnemonic ## InstructionId; \
    mnemonic##Instruction(uint32_t instruction): RISCVInstruction(instruction) {} \
    void accept(RISCVHart &hart, RISCVInterpreter &interpreter) override { \
        interpreter.visit ## mnemonic(hart, *this);                     \
    }                                                                   \
    const char * getMnemonic() const override {                              \
        return #mnemonic;                                               \
    }                                                                   \
    RISCVInstructionId getInstructionId() const override {              \
        return ID;                                                      \
    }                                                                   \
};

#include "InstructionTable.h"
#undef DEFINSTR

#endif
