// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <sstream>
#include <map>
#include <string>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <RISCVHart.hpp>
#include <RISCVInstruction.hpp>
#include <RISCVDecoder.hpp>
#include <RISCVInterpreter.hpp>
#include <ICacheBacking.hpp>
#include "SSTRISCVSimulator.hpp"
#include "SSTRISCVHart.hpp"
#include "DrvSysConfig.hpp"
#include "DrvAPIAddress.hpp"
#include "DrvAPIAddressMap.hpp"
#include "DrvStats.hpp"
namespace SST {
namespace Drv {

class RISCVCore : public SST::Component {
public:
    typedef Interfaces::StandardMem::Request Request;
    typedef std::function<void(Request *)> ICompletionHandler;

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        RISCVCore,
        "Drv",
        "RISCVCore",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "RISCV Core",
        COMPONENT_CATEGORY_PROCESSOR
    )
    // DOCUMENT PARAMETERS
    SST_ELI_DOCUMENT_PARAMS(
        /* input */
        {"program", "Program to run", "/path/to/r64elf"},
        {"load", "Load program into memory", "0"},
        {"release_reset", "Time to release from reset", "0"},
        /* control */
        {"mmio_addr_start", 0, "MMIO start address"},
        {"mmio_addr_end", 0, "MMIO end address"},
        /* system config */
        DRV_SYS_CONFIG_PARAMETERS
        {"sp", "[Core Value. ...]", ""},
        {"clock", "Clock rate in Hz", "1GHz"},
        {"num_harts", "Number of harts", "1"},
        {"core", "Core ID", "0"},
        {"pod", "Pod ID", "0"},
        {"pxn", "PXN ID", "0"},
        /* debugging */
        {"verbose", "Verbosity of output", "0"},
        {"debug_memory", "Debug memory requests", "0"},
        {"debug_idle", "Debug idle cycles", "0"},
        {"debug_requests", "Debug requests", "0"},
        {"debug_responses", "Debug responses", "0"},
        {"debug_syscalls", "Debug system calls", "0"},
        {"debug_mmio", "Debug MMIO requests", "0"},
        {"isa_test", "Report ISA tests results", "0"},
        {"test_name", "Optional name of the test", ""},
    )

    // Document the ports that this component accepts
    SST_ELI_DOCUMENT_PORTS(
       {"loopback", "A loopback link", {"Drv.DrvEvent", ""}},
    )

    // DOCUMENT SUBCOMPONENTS
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"memory", "Interface to a memory hierarchy", "SST::Interfaces::StandardMem"},
    )

    struct ThreadStats {
        std::vector<Statistic<uint64_t> *> instruction_count;
        Statistic<uint64_t> *load_l1sp;
        Statistic<uint64_t> *store_l1sp;
        Statistic<uint64_t> *atomic_l1sp;
        Statistic<uint64_t> *load_l2sp;
        Statistic<uint64_t> *store_l2sp;
        Statistic<uint64_t> *atomic_l2sp;
        Statistic<uint64_t> *load_dram;
        Statistic<uint64_t> *store_dram;
        Statistic<uint64_t> *atomic_dram;
        Statistic<uint64_t> *load_remote_pxn;
        Statistic<uint64_t> *store_remote_pxn;
        Statistic<uint64_t> *atomic_remote_pxn;
    };

    // DOCUMENT STATISTICS
    /* unfortunately the macro doesn't work with including "InstructionTable.h" */
    static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics()
    {
#define DEFINSTR(mnemonic, ...)                                         \
        {#mnemonic "_instruction", "Number of " #mnemonic " instructions", "instructions", 2},

        static std::vector<SST::ElementInfoStatistic> var    = {
#include <InstructionTable.h>
            {"load_l1sp", "Number of loads to local L1SP", "count", 1},
            {"store_l1sp", "Number of stores to local L1SP", "count", 1},
            {"atomic_l1sp", "Number of atomics to local L1SP", "count", 1},
            {"load_l2sp", "Number of loads to L2SP", "count", 1},
            {"store_l2sp", "Number of stores to L2SP", "count", 1},
            {"atomic_l2sp", "Number of atomics to L2SP", "count", 1},
            {"load_dram", "Number of loads to DRAM", "count", 1},
            {"store_dram", "Number of stores to DRAM", "count", 1},
            {"atomic_dram", "Number of atomics to DRAM", "count", 1},
            {"load_remote_pxn", "Number of loads to remote PXN", "count", 1},
            {"store_remote_pxn", "Number of stores to remote PXN", "count", 1},
            {"atomic_remote_pxn", "Number of atomics to remote PXN", "count", 1},
            {"stall_cycles", "Number of stalled cycles", "count", 1},
            {"busy_cycles", "Number of busy cycles", "count", 1},
        };

#undef DEFINSTR
#undef DEFINE_DRV_STAT
        auto parent = SST::ELI::InfoStats<
            std::conditional<(__EliDerivedLevel > __EliBaseLevel), __LocalEliBase, __ParentEliBase>::type>::get();
        SST::ELI::combineEliInfo(var, parent);
        return var;
    }

    /**
     * A key value pair
     */
    template <typename Key, typename Value>
    class KeyValue {
    public:
        KeyValue(std::string str) {
            std::stringstream ss(str);
            ss >> key >> value;
        }
        KeyValue(Key key, Value value) : key(key), value(value) {}
        Key   key;
        Value value;
    };


    /**
     * assert reset event
     */
    class AssertReset : public SST::Event {
    public:
        AssertReset() : SST::Event() {}
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
            Event::serialize_order(ser);
        }
        ImplementSerializable(SST::Drv::RISCVCore::AssertReset);
    };

    /**
     * deassert reset event
     */
    class DeassertReset : public SST::Event {
    public:
        DeassertReset() : SST::Event() {}
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
            Event::serialize_order(ser);
        }
        ImplementSerializable(SST::Drv::RISCVCore::DeassertReset);
    };

    /**
     * Constructor for RISCVCore
     */
    RISCVCore(SST::ComponentId_t id, SST::Params& params);

    /**
     * Destructor for RISCVCore
     */
    ~RISCVCore();

    /**
     * Init the simulation
     */
    void init(unsigned int phase) override;

    /**
     * Setup the simulation
     */
    void setup() override;

    /**
     * Finish the simulation
     */
    void finish() override;

    /**
     * Load a program segment
     */
    void loadProgramSegment(Elf64_Phdr* phdr);

    /**
     * Load a program
     */
    void loadProgram();

    static constexpr uint32_t DEBUG_MEMORY   = (1<< 0); //!< debug memory requests
    static constexpr uint32_t DEBUG_IDLE     = (1<< 1); //!< debug idle cycles
    static constexpr uint32_t DEBUG_SYSCALLS = (1<< 2); //!< debug system calls
    static constexpr uint32_t DEBUG_REQ      = (1<<30); //!< debug messages we expect to see when receiving requests
    static constexpr uint32_t DEBUG_RSP      = (1<<29); //!< debug messages we expect to see when receiving responses
    static constexpr uint32_t DEBUG_MMIO     = (1<<28); //!< debug messages we expect to see when receiving mmio requests

    /**
     * configure output stream
     */
    void configureOuptut(Params& params);

    /**
     * configure harts
     */
    void configureHarts(Params &params);

    /**
     * configure icache
     */
    void configureICache(Params &params);

    /**
     * configure memory
     */
    void configureMemory(Params &params);

    /**
     * configure clock
     */
    void configureClock(Params &params);

    /**
     * configure simulator
     */
    void configureSimulator(Params &params);

    /**
     * configure system config
     */
    void configureSysConfig(Params &params);

    /**
     * configure links
     */
    void configureLinks(Params &params);

    /**
     * clock tick
     */
    bool tick(Cycle_t cycle);

    /**
     * handle a memory event
     */
    void handleMemEvent(Interfaces::StandardMem::Request *req);

    /**
     * handle reset write
     */
    void handleResetWrite(uint64_t v);

    /**
     * handle a mmio write
     */
    void handleMMIOWrite(Interfaces::StandardMem::Write *write_req);

    /**
     * handle loopback event
     */
    void handleLoopback(Event *evt);

    /**
     * get the number of harts on this core
     */
    size_t numHarts() const { return harts_.size(); }


    int getHartId(RISCVSimHart &hart) const {
        return &hart - &harts_[0];
    }

    int getCoreId() const {
        return core_;
    }

    int getPodId() const {
        return pod_;
    }

    int getPXNId() const {
        return pxn_;
    }

    static constexpr int NO_HART = -1;
    /**
     * select the next hart to execute
     */
    int selectNextHart();

    /**
     * issue a memory request
     */
    void issueMemoryRequest(Request *req, int tid, ICompletionHandler &handler);

    /**
     * return true if we should exit
     */
    bool shouldExit() const {
        for (auto &hart : harts_) {
            if (!hart.exit())
                return false;
        }
        return true;
    }

    void profileInstruction(RISCVSimHart &hart, RISCVInstruction &instruction) {
#ifdef SST_RISCV_CORE_PROFILE_INSTRUCTIONS
        auto it = pchist_.find(hart.pc());
        if (it == pchist_.end()) {
            pchist_[hart.pc()] = 1;
        } else {
            it->second++;
        }
#endif
    }

    /**
     * get system info
     */
    DrvAPI::DrvAPISysConfig sys() const { return sys_config_.config(); }

    /**
     * get the max write request size
     */
    size_t getMaxReqSize() const { return sys().numNWObufDwords() * sizeof(uint64_t); }

    /**
     * decode a virtual address to a physical address
     */
    DrvAPI::DrvAPIPAddress toPhysicalAddress(uint64_t addr) const;

    /**
     * is local l1sp for purpose of stats
     */
    bool isPAddressL1SP(DrvAPI::DrvAPIPAddress addr) const {
        return addr.type() == DrvAPI::DrvAPIPAddress::TYPE_L1SP
            && addr.pxn() == static_cast<uint64_t>(pxn_);
    }

    /**
     * is  l2sp for purpose of stats
     */
    bool isPAddressL2SP(DrvAPI::DrvAPIPAddress addr) const {
        return addr.type() == DrvAPI::DrvAPIPAddress::TYPE_L2SP
            && addr.pxn() == static_cast<uint64_t>(pxn_);
    }

    /**
     * is  dram for purpose of stats
     */
    bool isPAddressDRAM(DrvAPI::DrvAPIPAddress addr) const {
        return addr.type() == DrvAPI::DrvAPIPAddress::TYPE_DRAM
            && addr.pxn() == static_cast<uint64_t>(pxn_);
    }

    /**
     * is remote pxn memory for purpose of stats
     */
    bool isPAddressRemotePXN(DrvAPI::DrvAPIPAddress addr) const {
        return addr.pxn() != static_cast<uint64_t>(pxn_);
    }

    /**
     * add load statistic
     */
    void addLoadStat(DrvAPI::DrvAPIPAddress addr, RISCVSimHart &hart) {
        int id = getHartId(hart);
        ThreadStats &stats = thread_stats_[id];
        if (isPAddressL1SP(addr)) {
            stats.load_l1sp->addData(1);
        } else if (isPAddressL2SP(addr)) {
            stats.load_l2sp->addData(1);
        } else if (isPAddressDRAM(addr)) {
            stats.load_dram->addData(1);
        } else if (isPAddressRemotePXN(addr)) {
            stats.load_remote_pxn->addData(1);
        }
    }

    /**
     * add store statistic
     */
    void addStoreStat(DrvAPI::DrvAPIPAddress addr, RISCVSimHart &hart) {
        int id = getHartId(hart);
        ThreadStats &stats = thread_stats_[id];
        if (isPAddressL1SP(addr)) {
            stats.load_l1sp->addData(1);
        } else if (isPAddressL2SP(addr)) {
            stats.load_l2sp->addData(1);
        } else if (isPAddressDRAM(addr)) {
            stats.load_dram->addData(1);
        } else if (isPAddressRemotePXN(addr)) {
            stats.load_remote_pxn->addData(1);
        }
    }

    /**
     * add atomic statistic
     */
    void addAtomicStat(DrvAPI::DrvAPIPAddress addr, RISCVSimHart &hart) {
        int id = getHartId(hart);
        ThreadStats &stats = thread_stats_[id];
        if (isPAddressL1SP(addr)) {
            stats.atomic_l1sp->addData(1);
        } else if (isPAddressL2SP(addr)) {
            stats.atomic_l2sp->addData(1);
        } else if (isPAddressDRAM(addr)) {
            stats.atomic_dram->addData(1);
        } else if (isPAddressRemotePXN(addr)) {
            stats.atomic_remote_pxn->addData(1);
        }
    }

    void addBusyCycleStat(uint64_t cycles) {
        busy_cycles_->addData(cycles);
    }

    void addStallCycleStat(uint64_t cycles) {
        stall_cycles_->addData(cycles);
    }

    /**
     * configure statistics
     */
    void configureStatistics(Params &params);

    /**
     * test name
     */
    std::string testName() const { return test_name_; }

    SST::Output output_; //!< output stream
    SST::Output isa_test_output_; //!< isa test output stream
    std::string test_name_; //!< test name
    Interfaces::StandardMem *mem_; //!< memory interface
    RISCVSimulator *sim_; //!< simulator
    ICacheBacking *icache_; //!< icache
    RISCVDecoder decoder_; //!< decoder
    std::vector<RISCVSimHart> harts_; //!< harts
    std::map<int, ICompletionHandler> rsp_handlers_; //!< response handlers
    SST::TimeConverter *clocktc_; //!< the clock time converter
    int last_hart_; //!< last hart to execute
    bool load_program_; //!< load program
    DrvSysConfig sys_config_; //!< system configuration
    std::map<uint64_t, int64_t> pchist_; //!< program counter history
    int core_; //!< core id wrt pod
    int pod_;  //!< pod id wrt pxn
    int pxn_;  //!< pxn id wrt system
    uint64_t reset_time_; //!< reset time
    // statistics
    std::vector<ThreadStats> thread_stats_; //!< thread stats
    Statistic<uint64_t> *busy_cycles_; //!< cycle count
    Statistic<uint64_t> *stall_cycles_; //!< stall cycle count
    DrvAPI::DrvAPIPAddress mmio_start_; //!< mmio start address
    DrvAPI::DrvAPIPAddress mmio_end_; //!< mmio end address
    SST::Link *loopback_; //!< loopback link
};


}
}
