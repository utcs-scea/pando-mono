// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RV64IMFINTERPRETER_HPP
#define RV64IMFINTERPRETER_HPP
#include "RISCVInstruction.hpp"
#include "RISCVHart.hpp"
#include "RV64IMInterpreter.hpp"
#include <stdexcept>
#include <boost/multiprecision/cpp_int.hpp>
#include <cmath>
#include <cfenv>

class RV64IMFInterpreter : public RV64IMInterpreter
{
public:
    RV64IMFInterpreter(): RV64IMInterpreter() {}

    /**
     * @brief temporarily changes rounding mode while in scope
     */
    struct RoundingModeGuard {
        RoundingModeGuard(int rm) {
            old_rounding_mode = fegetround();
            fesetround(rm);
        }
        ~RoundingModeGuard() {
            fesetround(old_rounding_mode);
        }
        int old_rounding_mode;
    };


    void visitFMADD_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = std::fmaf(hart.sf(i.rs1()),
                                   hart.sf(i.rs2()),
                                   hart.sf(i.rs3()));
        hart.pc() += 4;
    }

    void visitFMSUB_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = std::fmaf(hart.sf(i.rs1()),
                                   hart.sf(i.rs2()),
                                   -hart.sf(i.rs3()));
        hart.pc() += 4;
    }

    void visitFNMSUB_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = std::fmaf(-hart.sf(i.rs1()),
                                   hart.sf(i.rs2()),
                                   hart.sf(i.rs3()));
        hart.pc() += 4;
    }

    void visitFNMADD_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = std::fmaf(-hart.sf(i.rs1()),
                                   hart.sf(i.rs2()),
                                   -hart.sf(i.rs3()));
        hart.pc() += 4;
    }

    void visitFADD_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = hart.sf(i.rs1()) + hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFSUB_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = hart.sf(i.rs1()) - hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFMUL_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = hart.sf(i.rs1()) * hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFDIV_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = hart.sf(i.rs1()) / hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFSQRT_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        hart.f(i.rd()) = std::sqrt(hart.sf(i.rs1()));
        hart.pc() += 4;
    }

    void visitFSGNJ_S(RISCVHart &hart, RISCVInstruction &i) override {
        hart.f(i.rd()) = std::copysignf(hart.sf(i.rs1()), hart.sf(i.rs2()));
        hart.pc() += 4;
    }

    void visitFSGNJN_S(RISCVHart &hart, RISCVInstruction &i) override {
        hart.f(i.rd()) = std::copysignf(hart.sf(i.rs1()), -hart.sf(i.rs2()));
        hart.pc() += 4;
    }

    void visitFSGNJX_S(RISCVHart &hart, RISCVInstruction &i) override {
        float s1 = std::copysignf(1.0, hart.sf(i.rs1()));
        float s2 = std::copysignf(1.0, hart.sf(i.rs2()));
        hart.f(i.rd()) = hart.sf(i.rs1()) * s2 * s1;
        hart.pc() += 4;
    }

    void visitFMIN_S(RISCVHart &hart, RISCVInstruction &i) override {
        hart.f(i.rd()) = std::fminf(hart.sf(i.rs1()), hart.sf(i.rs2()));
        hart.pc() += 4;
    }

    void visitFMAX_S(RISCVHart &hart, RISCVInstruction &i) override {
        hart.f(i.rd()) = std::fmaxf(hart.sf(i.rs1()), hart.sf(i.rs2()));
        hart.pc() += 4;
    }

    void visitFCVT_W_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        int32_t result = std::rintf(hart.sf(i.rs1()));
        hart.sx(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_L_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        int64_t result = std::rintf(hart.sf(i.rs1()));
        hart.sx(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_WU_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        uint32_t result = std::rintf(hart.sf(i.rs1()));
        hart.x(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_LU_S_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        uint64_t result = std::rintf(hart.sf(i.rs1()));
        hart.x(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_S_W_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        int32_t result = static_cast<int32_t>(hart.x(i.rs1()));
        hart.f(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_S_L_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        int64_t result = static_cast<int64_t>(hart.x(i.rs1()));
        hart.f(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFCVT_S_WU_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        uint32_t result = static_cast<uint32_t>(hart.x(i.rs1()));
        hart.f(i.rd()) = result;
        hart.pc() += 4;
    }


    void visitFCVT_S_LU_DYN(RISCVHart &hart, RISCVInstruction &i) override {
        RoundingModeGuard guard(hart.rm());
        uint64_t result = static_cast<uint64_t>(hart.x(i.rs1()));
        hart.f(i.rd()) = result;
        hart.pc() += 4;
    }

    void visitFMV_X_W(RISCVHart &hart, RISCVInstruction &i) override {
        union {
            uint32_t u_;
            float    f_;
        } u;
        u.f_ = hart.sf(i.rs1());
        hart.sx(i.rd()) = u.u_;
        hart.pc() += 4;
    }

    void visitFMV_W_X(RISCVHart &hart, RISCVInstruction &i) override {
        union {
            uint32_t u_;
            float    f_;
        } u;
        u.u_ = { static_cast<uint32_t>(hart.x(i.rs1())) };
        hart.sf(i.rd()) = u.f_;
        hart.pc() += 4;
    }

    void visitFEQ_S(RISCVHart &hart, RISCVInstruction &i) override {
        float f1 = hart.sf(i.rs1());
        float f2 = hart.sf(i.rs2());
        // TODO: signal nan in control register when we implement that
        hart.x(i.rd())
            = std::isnan(f1)||std::isnan(f2)
            ? 0
            : hart.sf(i.rs1()) == hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFLT_S(RISCVHart &hart, RISCVInstruction &i) override {
        float f1 = hart.sf(i.rs1());
        float f2 = hart.sf(i.rs2());
        if (std::isnan(f1) || std::isnan(f2)) {
            std::string which = std::isnan(f1) ? "rs1" : "rs2";
            throw std::runtime_error("FLT_S: nan in " + which);
        }
        hart.x(i.rd()) = hart.sf(i.rs1()) < hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFLE_S(RISCVHart &hart, RISCVInstruction &i) override {
        float f1 = hart.sf(i.rs1());
        float f2 = hart.sf(i.rs2());
        if (std::isnan(f1) || std::isnan(f2)) {
            std::string which = std::isnan(f1) ? "rs1" : "rs2";
            throw std::runtime_error("FLE_S: nan in " + which);
        }
        hart.x(i.rd()) = hart.sf(i.rs1()) <= hart.sf(i.rs2());
        hart.pc() += 4;
    }

    void visitFCLASS_S(RISCVHart &hart, RISCVInstruction &i) override {
        float f = hart.sf(i.rs1());
        uint64_t result = 0;
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_NEG_INF,
                          std::isinf(f) && std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_POS_INF,
                          std::isinf(f) && !std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_NEG_NORMAL,
                          std::fpclassify(f) == FP_NORMAL && std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_POS_NORMAL,
                          std::fpclassify(f) == FP_NORMAL && !std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_NEG_SUBNORMAL,
                          std::fpclassify(f) == FP_SUBNORMAL && std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_POS_SUBNORMAL,
                          std::fpclassify(f) == FP_SUBNORMAL && !std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_NEG_ZERO,
                          std::fpclassify(f) == FP_ZERO && std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_POS_ZERO,
                          std::fpclassify(f) == FP_ZERO && !std::signbit(f));
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_SIGNAL_NAN,
                          false);
        riscvbits::setbit(result, RISCVHart::FCLASS_IS_QUIET_NAN,
                          std::isnan(f));
        hart.x(i.rd()) = result;
        hart.pc() += 4;
    }
};
#endif
