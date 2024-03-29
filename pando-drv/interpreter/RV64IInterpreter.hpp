// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RV64IINTERPRETER_HPP
#define RV64IINTERPRETER_HPP
#include "RISCVHart.hpp"
#include "RISCVInstruction.hpp"
#include "RISCVInterpreter.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>

class RV64IInterpreter : public RISCVInterpreter {
public:
  RV64IInterpreter() {}
  void visitLUI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.sx(i.rd()) = static_cast<int64_t>(i.SUimm());
    hart.pc() += 4;
  }
  void visitAUIPC(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.pc() + static_cast<int64_t>(i.SUimm());
    hart.pc() += 4;
  }
  void visitJAL(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.pc() + 4;
    hart.pc() += i.Jimm();
  }
  void visitJALR(RISCVHart& hart, RISCVInstruction& i) override {
    uint64_t jmp_target = (hart.x(i.rs1()) + i.SIimm());
    hart.x(i.rd()) = hart.pc() + 4;
    hart.pc() = jmp_target;
  }
  void visitBEQ(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.x(i.rs1()) == hart.x(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }
  void visitBNE(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.x(i.rs1()) != hart.x(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }
  void visitBLT(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.sx(i.rs1()) < hart.sx(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }
  void visitBGE(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.sx(i.rs1()) >= hart.sx(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }
  void visitBLTU(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.x(i.rs1()) < hart.x(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }
  void visitBGEU(RISCVHart& hart, RISCVInstruction& i) override {
    if (hart.x(i.rs1()) >= hart.x(i.rs2())) {
      hart.pc() += i.Bimm();
    } else {
      hart.pc() += 4;
    }
  }

  // skip loads for now
  void visitADDI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) + i.SIimm();
    hart.pc() += 4;
  }
  void visitSLTI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) < i.SIimm();
    hart.pc() += 4;
  }
  void visitSLTIU(RISCVHart& hart, RISCVInstruction& i) override {
    uint64_t imm = i.SIimm(); // sign extend
    hart.x(i.rd()) = hart.x(i.rs1()) < imm;
    hart.pc() += 4;
  }
  void visitXORI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) ^ i.SIimm();
    hart.pc() += 4;
  }
  void visitORI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) | i.SIimm();
    hart.pc() += 4;
  }
  void visitANDI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) & i.SIimm();
    hart.pc() += 4;
  }
  void visitSLLI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) << i.shamt6();
    hart.pc() += 4;
  }
  void visitSRLI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) >> i.shamt6();
    hart.pc() += 4;
  }
  void visitSRAI(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) >> i.shamt6();
    hart.pc() += 4;
  }
  void visitADD(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) + hart.sx(i.rs2());
    hart.pc() += 4;
  }
  void visitSUB(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) - hart.sx(i.rs2());
    hart.pc() += 4;
  }
  void visitSLL(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) << hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitSLT(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) < hart.sx(i.rs2());
    hart.pc() += 4;
  }
  void visitSLTU(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) < hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitXOR(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) ^ hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitSRL(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) >> hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitSRA(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.sx(i.rs1()) >> hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitOR(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) | hart.x(i.rs2());
    hart.pc() += 4;
  }
  void visitAND(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = hart.x(i.rs1()) & hart.x(i.rs2());
    hart.pc() += 4;
  }

  // skip fence for now
  // skip fence.i for now
  // skip ecall and ebreak for now
  // skip csr ops for now

  void visitADDIW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    int32_t rs = hart.sx(i.rs1());
    int32_t rd = rs + i.SIimm();
    hart.sx(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSLLIW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    uint32_t rs = hart.sx(i.rs1());
    int32_t rd = rs << i.shamt5();
    hart.sx(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSRLIW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    uint32_t rs = hart.x(i.rs1());
    int32_t rd = rs >> i.shamt5();
    hart.sx(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSRAIW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    int32_t rs = hart.sx(i.rs1());
    int32_t rd = rs >> i.shamt5();
    hart.sx(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitADDW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    int32_t rs1 = hart.sx(i.rs1());
    int32_t rs2 = hart.sx(i.rs2());
    int32_t rd = rs1 + rs2;
    // std::cout << "ADDW: rs1 = " << rs1 << ", rs2 = " << rs2 << ", rd = " << rd << std::endl;
    hart.x(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSUBW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    int32_t rs1 = hart.sx(i.rs1());
    int32_t rs2 = hart.sx(i.rs2());
    int32_t rd = rs1 - rs2;
    hart.x(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSLLW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    uint32_t rs1 = hart.sx(i.rs1());
    uint32_t rs2 = hart.sx(i.rs2());
    int32_t rd = rs1 << rs2;
    hart.x(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSRLW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    uint32_t rs1 = hart.sx(i.rs1());
    uint32_t rs2 = hart.sx(i.rs2());
    int32_t rd = rs1 >> rs2;
    hart.x(i.rd()) = rd;
    hart.pc() += 4;
  }
  void visitSRAW(RISCVHart& hart, RISCVInstruction& i) override {
    // truncate to signed 32 bits
    int32_t rs1 = hart.sx(i.rs1());
    int32_t rs2 = hart.sx(i.rs2());
    int32_t rd = rs1 >> rs2;
    hart.x(i.rd()) = rd;
    hart.pc() += 4;
  }
};

#endif
