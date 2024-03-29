// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "ICacheBacking.hpp"
#include "RISCVDecoder.hpp"
#include "RISCVHart.hpp"
#include "RISCVInstruction.hpp"
#include "RISCVInterpreter.hpp"
#include "RV64IMInterpreter.hpp"
#include <iomanip>
#include <iostream>

class MyInterpreter : public RV64IMInterpreter {
public:
  MyInterpreter() : mem(4 * 1024) {}

  template <typename R, typename T>
  R visitLoad(RISCVHart& hart, RISCVInstruction& i) {
    T* ptr = (T*)&mem[hart.x(i.rs1()) + i.SIimm()];
    return static_cast<R>(*ptr);
  }

  template <typename T>
  void visitStore(RISCVHart& hart, RISCVInstruction& i) {
    T* ptr = (T*)&mem[hart.x(i.rs1()) + i.Simm()];
    *ptr = static_cast<T>(hart.x(i.rs2()));
  }

  void visitLB(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<int64_t, int8_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLH(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<int64_t, int16_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLW(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<int64_t, int32_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLBU(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<uint64_t, uint8_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLHU(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<uint64_t, uint16_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLWU(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<uint64_t, uint32_t>(hart, i);
    hart.pc() += 4;
  }
  void visitLD(RISCVHart& hart, RISCVInstruction& i) override {
    hart.x(i.rd()) = visitLoad<uint64_t, uint64_t>(hart, i);
    hart.pc() += 4;
  }

  void visitSB(RISCVHart& hart, RISCVInstruction& i) override {
    visitStore<uint8_t>(hart, i);
    hart.pc() += 4;
  }
  void visitSH(RISCVHart& hart, RISCVInstruction& i) override {
    visitStore<uint16_t>(hart, i);
    hart.pc() += 4;
  }
  void visitSW(RISCVHart& hart, RISCVInstruction& i) override {
    visitStore<uint32_t>(hart, i);
    hart.pc() += 4;
  }
  void visitSD(RISCVHart& hart, RISCVInstruction& i) override {
    visitStore<uint64_t>(hart, i);
    hart.pc() += 4;
  }

  std::vector<uint8_t> mem;
};

int main(int argc, char* argv[]) {
  int arg = 1;
  const char* filename = "test.riscv";
  if (arg < argc) {
    filename = argv[1];
  }
  ICacheBacking* icache = new ICacheBacking(filename);
  icache->printProgramHeaders();
  uint32_t pc = icache->getStartAddr();
  std::cout << "start address = " << std::hex << pc << std::endl;
  RISCVHart* hart = new RISCVHart();
  RISCVDecoder* decoder = new RISCVDecoder();
  RISCVInterpreter* interpreter = new MyInterpreter();
  hart->pc() = pc;

  for (int i = 0; i < 200; i++) {
    RISCVInstruction* instr = decoder->decode(icache->read(hart->pc()));
    std::cout << "instruction = " << instr->getMnemonic() << std::endl;
    std::cout << "            = " << std::hex << std::setw(8) << std::setfill('0')
              << instr->instruction() << std::endl;
    interpreter->visit(*hart, *instr);
  }

  std::cout << hart->to_string() << std::endl;
  return 0;
}
