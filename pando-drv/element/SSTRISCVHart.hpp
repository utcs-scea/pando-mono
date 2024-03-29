// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <RISCVHart.hpp>
namespace SST {
namespace Drv {

class RISCVSimHart : public RISCVHart {
public:

    virtual ~RISCVSimHart() {}

    /**
     * @brief ready
     */
    bool ready() { return !reset() && !stalledMemory(); }

    /**
     * @brief reset
     */
    class reset_handle {
    public:
        reset_handle(RISCVSimHart &hart) : _hart(hart) {}
        reset_handle(const reset_handle &other) = delete;
        reset_handle(reset_handle &&other) = delete;
        reset_handle & operator=(const reset_handle &other) = delete;
        reset_handle & operator=(reset_handle &&other) = delete;

        operator bool() const {
            return _hart._reset;
        }
        reset_handle & operator=(bool reset) {
            _hart._reset = reset;
            if (reset) {
                _hart.pc() = _hart.resetPC();
                _hart.exitCode() = 0;
                _hart.stalledMemory() = false;
            }
            return *this;
        }
    private:
        RISCVSimHart &_hart;
    };

    class const_reset_handle {
    public:
        const_reset_handle(const RISCVSimHart &hart) : _hart(hart) {}
        operator bool() const {
            return _hart._reset;
        }
    private:
        const RISCVSimHart &_hart;
    };
    reset_handle reset() { return reset_handle(*this); }
    const_reset_handle reset() const { return const_reset_handle(*this); }

    /**
     * @brief stalledMemory
     */
    bool & stalledMemory() { return _stalled_memory; }
    bool   stalledMemory() const { return _stalled_memory; }

    /**
     * @brief exit
     */
    int & exit() { return _exit; }
    int   exit() const { return _exit; }

    /**
     * @brief exitCode
     */
    int64_t & exitCode() { return _exit_code; }
    int64_t   exitCode() const { return _exit_code; }

    /**
     * @brief resetPC
     */
    uint64_t & resetPC() { return _reset_pc; }
    uint64_t   resetPC() const { return _reset_pc; }

    bool _stalled_memory = false;
    bool _reset = false;
    int  _exit = false;
    int64_t _exit_code = 0;
    uint64_t _reset_pc = 0;
};

}
}
