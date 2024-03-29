// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RV64IMINTERPRETER_HPP
#define RV64IMINTERPRETER_HPP
#include "RISCVInstruction.hpp"
#include "RISCVHart.hpp"
#include "RV64IInterpreter.hpp"
#include <stdexcept>
#include <boost/multiprecision/cpp_int.hpp>
class RV64IMInterpreter : public RV64IInterpreter
{
public:
    RV64IMInterpreter(){}
    using int128_t = boost::multiprecision::int128_t;
    using uint128_t = boost::multiprecision::uint128_t;
    void visitMUL(RISCVHart &hart, RISCVInstruction &i) override {
        int128_t rs1 = static_cast<int64_t>(hart.sx(i.rs1()));
        int128_t rs2 = static_cast<int64_t>(hart.sx(i.rs2()));
        int128_t rd = rs1 * rs2;
        hart.x(i.rd()) = static_cast<int64_t>(rd & std::numeric_limits<uint64_t>::max());
        hart.pc() += 4;
    }
    void visitMULH(RISCVHart &hart, RISCVInstruction &i) override {
        int128_t rs1 = static_cast<int64_t>(hart.sx(i.rs1()));
        int128_t rs2 = static_cast<int64_t>(hart.sx(i.rs2()));
        int128_t rd = rs1 * rs2;
        hart.x(i.rd()) = static_cast<int64_t>(rd >> 64);
        hart.pc() += 4;
    }
    void visitMULHU(RISCVHart &hart, RISCVInstruction &i) override {
        uint128_t rs1 = static_cast<uint64_t>(hart.x(i.rs1()));
        uint128_t rs2 = static_cast<uint64_t>(hart.x(i.rs2()));
        uint128_t rd = rs1 * rs2;
        hart.x(i.rd()) = static_cast<uint64_t>(rd >> 64);
        hart.pc() += 4;
    }
    void visitMULHSU(RISCVHart &hart, RISCVInstruction &i) override {
        int128_t rs1 = static_cast<int64_t>(hart.sx(i.rs1()));
        uint128_t rs2 = static_cast<uint64_t>(hart.x(i.rs2()));
        int128_t rd = rs1 * rs2;
        hart.x(i.rd()) = static_cast<int64_t>(rd >> 64);
        hart.pc() += 4;
    }
    void visitDIV(RISCVHart &hart, RISCVInstruction &i) override {
        int64_t rs1 = hart.sx(i.rs1());
        int64_t rs2 = hart.sx(i.rs2());
        // check for division by zero and overflow
        if (rs2 == 0) {
            hart.x(i.rd()) = -1;
        } else if (rs1 == std::numeric_limits<int64_t>::min() && rs2 == -1) {
            hart.x(i.rd()) = std::numeric_limits<int64_t>::min();
        } else {
            hart.x(i.rd()) = rs1 / rs2;
        }
        hart.pc() += 4;
    }
    void visitDIVU(RISCVHart &hart, RISCVInstruction &i) override {
        uint64_t rs1 = hart.x(i.rs1());
        uint64_t rs2 = hart.x(i.rs2());
        if (rs2 == 0) {
            hart.x(i.rd()) = std::numeric_limits<uint64_t>::max();
        } else {
            hart.x(i.rd()) = rs1 / rs2;
        }
        hart.pc() += 4;
    }
    void visitREM(RISCVHart &hart, RISCVInstruction &i) override {
        int64_t rs1 = hart.sx(i.rs1());
        int64_t rs2 = hart.sx(i.rs2());
        // check for division by zero and overflow
        if (rs2 == 0) {
            hart.x(i.rd()) = rs1;
        } else if (rs1 == std::numeric_limits<int64_t>::min() && rs2 == -1) {
            hart.x(i.rd()) = 0;
        } else {
            hart.x(i.rd()) = rs1 % rs2;
        }
        hart.pc() += 4;
    }
    void visitREMU(RISCVHart &hart, RISCVInstruction &i) override {
        uint64_t rs1 = hart.x(i.rs1());
        uint64_t rs2 = hart.x(i.rs2());
        // check for division by zero
        if (rs2 == 0) {
            hart.x(i.rd()) = rs1;
        } else {
            hart.x(i.rd()) = rs1 % rs2;
        }
        hart.pc() += 4;
    }
    void visitMULW(RISCVHart &hart, RISCVInstruction &i) override {
        int32_t rs1 = hart.sx(i.rs1());
        int32_t rs2 = hart.sx(i.rs2());
        int64_t rd = rs1 * rs2;
        hart.x(i.rd()) = rd;
        hart.pc() += 4;
    }
    void visitDIVW(RISCVHart &hart, RISCVInstruction &i) override {
        int32_t rs1 = hart.sx(i.rs1());
        int32_t rs2 = hart.sx(i.rs2());
        // check for division by zero and overflow
        if (rs2 == 0) {
            hart.x(i.rd()) = -1;
        } else if (rs1 == std::numeric_limits<int32_t>::min() && rs2 == -1) {
            hart.x(i.rd()) = std::numeric_limits<int32_t>::min();
        } else {
            hart.x(i.rd()) = rs1 / rs2;
        }
        hart.pc() += 4;
    }
    void visitDIVUW(RISCVHart &hart, RISCVInstruction &i) override {
        uint32_t rs1 = hart.x(i.rs1());
        uint32_t rs2 = hart.x(i.rs2());
        if (rs2 == 0) {
            int32_t rd = std::numeric_limits<uint32_t>::max();
            hart.x(i.rd()) = rd;
        } else {
            int32_t rd = rs1 / rs2;
            hart.sx(i.rd()) = rd;
        }
        hart.pc() += 4;
    }
    void visitREMW(RISCVHart &hart, RISCVInstruction &i) override {
        int32_t rs1 = hart.sx(i.rs1());
        int32_t rs2 = hart.sx(i.rs2());
        // check for division by zero and overflow
        if (rs2 == 0) {
            hart.x(i.rd()) = rs1;
        } else if (rs1 == std::numeric_limits<int32_t>::min() && rs2 == -1) {
            hart.x(i.rd()) = 0;
        } else {
            hart.x(i.rd()) = rs1 % rs2;
        }
        hart.pc() += 4;
    }
    void visitREMUW(RISCVHart &hart, RISCVInstruction &i) override {
        uint32_t rs1 = hart.sx(i.rs1());
        uint32_t rs2 = hart.sx(i.rs2());
        if (rs2 == 0) {
            int32_t rd = rs1;
            hart.x(i.rd()) = rd;
        } else {
            int32_t rd = rs1 % rs2;
            hart.x(i.rd()) = rd;
        }
        hart.pc() += 4;
    }
};
#endif
