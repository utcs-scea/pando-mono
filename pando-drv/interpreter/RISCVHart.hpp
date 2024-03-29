// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVHART_HPP
#define RISCVHART_HPP
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cfenv>
#include <cstring>

namespace riscvbits
{
/**
 * @brief set a bit in a value
 */
template <typename UINT>
static inline void setbit(UINT &val, UINT bit, bool set) {
    val = (val & ~(1 << bit)) | ((UINT)set << bit);
}
} // namespace riscvbits

class RISCVHart {
public:
    using InternalFPType = float;

    uint64_t        _x[32];
    InternalFPType  _f[32];
    int             _rm;
    uint64_t        _pc;

    /**
     * @brief floating point class bitmasks
     */
    static constexpr uint64_t FCLASS_IS_NEG_INF       = 1<<0;
    static constexpr uint64_t FCLASS_IS_NEG_NORMAL    = 1<<1;
    static constexpr uint64_t FCLASS_IS_NEG_SUBNORMAL = 1<<2;
    static constexpr uint64_t FCLASS_IS_NEG_ZERO      = 1<<3;
    static constexpr uint64_t FCLASS_IS_POS_ZERO      = 1<<4;
    static constexpr uint64_t FCLASS_IS_POS_SUBNORMAL = 1<<5;
    static constexpr uint64_t FCLASS_IS_POS_NORMAL    = 1<<6;
    static constexpr uint64_t FCLASS_IS_POS_INF       = 1<<7;
    static constexpr uint64_t FCLASS_IS_SIGNAL_NAN    = 1<<8;
    static constexpr uint64_t FCLASS_IS_QUIET_NAN     = 1<<9;

    RISCVHart()
        : _rm(FE_TONEAREST)
        , _pc(0) {
        memset(_x, 0, sizeof(_x));
        memset(_f, 0, sizeof(_f));
    }

    /**
     * mutable reference
     */
    template <typename ScalarT>
    class reference_handle {
    public:
        reference_handle(ScalarT &ref, bool zero = false)
            : _ref(ref)
            , _zero(zero) {
        }
        operator ScalarT() const { return _zero ? static_cast<ScalarT>(0) : _ref; }

        reference_handle &operator=(ScalarT val) {
            if (!_zero) _ref = val;
            return *this;
        }

        reference_handle &operator+=(ScalarT val) {
            if (!_zero) _ref += val;
            return *this;
        }

        ScalarT &_ref;
        bool _zero;
    };

    /**
     * const reference
     */
    template <typename ScalarT>
    class const_reference_handle {
    public:
        const_reference_handle(const ScalarT &ref, bool zero = false)
            : _ref(ref)
            , _zero(zero) {
        }
        operator ScalarT() const { return _zero ? static_cast<ScalarT>(0) : _ref; }
        const ScalarT &_ref;
        bool _zero;
    };

    /**
     * Mutable reference for floating point types
     */
    template <typename FPType>
    class fp_reference_handle {
    public:
        fp_reference_handle(InternalFPType &ref)
            : _ref(ref) {
        }

        operator FPType() const { return static_cast<FPType>(_ref); }

        fp_reference_handle &operator=(FPType val) {
            _ref = static_cast<InternalFPType>(val);
            return *this;
        }

        InternalFPType &_ref;
    };

    /**
     * Const reference for floating point types
     */
    template <typename FPType>
    class const_fp_reference_handle {
    public:
        const_fp_reference_handle(const InternalFPType &ref)
            : _ref(ref) {
        }

        operator FPType() const { return static_cast<FPType>(_ref); }

        const InternalFPType &_ref;
    };

    template <typename IdxT>
    reference_handle<uint64_t> x(IdxT i) {
        return reference_handle<uint64_t>(_x[i], i == 0);
    }

    template <typename IdxT>
    const const_reference_handle<uint64_t> x(IdxT i) const {
        return const_reference_handle<uint64_t>(_x[i], i == 0);
    }

    template <typename IdxT>
    reference_handle<int64_t> sx(IdxT i) {
        return reference_handle<int64_t>(reinterpret_cast<int64_t &>(_x[i]), i == 0);
    }

    template <typename IdxT>
    const const_reference_handle<int64_t> sx(IdxT i) const {
        return const_reference_handle<int64_t>(reinterpret_cast<int64_t &>(_x[i]), i == 0);
    }

    template <typename IdxT>
    reference_handle<uint64_t> a(IdxT i) {
        assert(i < 8);
        return x(10 + i);
    }

    template <typename IdxT>
    const const_reference_handle<uint64_t> a(IdxT i) const {
        assert(i < 8);
        return x(10 + i);
    }

    template <typename IdxT>
    reference_handle<int64_t> sa(IdxT i) {
        assert(i < 8);
        return sx(10 + i);
    }

    template <typename IdxT>
    const const_reference_handle<int64_t> sa(IdxT i) const {
        assert(i < 8);
        return sx(10 + i);
    }


    template <typename IdxT>
    fp_reference_handle<InternalFPType> f(IdxT i) {
        assert(i < 32);
        return fp_reference_handle<InternalFPType>(_f[i]);
    }

    template <typename IdxT>
    const const_fp_reference_handle<InternalFPType> f(IdxT i) const {
        assert(i < 32);
        return const_fp_reference_handle<InternalFPType>(_f[i]);
    }

    template <typename IdxT>
    fp_reference_handle<float> sf(IdxT i) {
        assert(i < 32);
        return fp_reference_handle<float>(_f[i]);
    }

    template <typename IdxT>
    const const_fp_reference_handle<float> sf(IdxT i) const {
        assert(i < 32);
        return const_fp_reference_handle<float>(_f[i]);
    }

    template <typename IdxT>
    fp_reference_handle<double> df(IdxT i) {
        assert(i < 32);
        return fp_reference_handle<double>(_f[i]);
    }

    template <typename IdxT>
    const const_fp_reference_handle<double> sf(IdxT i) const {
        assert(i < 32);
        return const_fp_reference_handle<double>(_f[i]);
    }

    reference_handle<uint64_t> pc() {
        return reference_handle<uint64_t>(_pc, false);
    }

    const const_reference_handle<uint64_t> pc() const {
        return const_reference_handle<uint64_t>(_pc, false);
    }

    reference_handle<uint64_t> sp() {
        return x(2);
    }

    const const_reference_handle<uint64_t> sp() const {
        return x(2);
    }

    int &rm() {
        return _rm;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "pc: " << std::hex << pc() << std::endl;
        for (int i = 0; i < 32; i++) {
            ss << "x[" << std::setw(2) << std::dec << i << "]: ";
            ss << std::hex << x(i) << std::endl;
        }
        for (int i = 0; i < 32; i++) {
            ss << "f[" << std::setw(2) << std::dec << i << "]: ";
            ss << f(i) << std::endl;
        }
        return ss.str();
    }
};

#endif
