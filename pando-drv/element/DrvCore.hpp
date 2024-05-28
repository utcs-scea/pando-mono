// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/event.h>
#include <memory>
#include "DrvEvent.hpp"
#include "DrvMemory.hpp"
#include "DrvThread.hpp"
#include "DrvSystem.hpp"
#include "DrvSysConfig.hpp"
#include "DrvAPIMain.hpp"
#include "DrvStats.hpp"
#include <DrvAPI.hpp>
namespace SST {
namespace Drv {

/**
 * A Core
 */
class DrvCore : public SST::Component {
public:
  // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
  SST_ELI_REGISTER_COMPONENT(
      DrvCore,
      "Drv",
      "DrvCore",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Drv Core",
      COMPONENT_CATEGORY_UNCATEGORIZED
  )
  // Document the parameters that this component accepts
  SST_ELI_DOCUMENT_PARAMS(
      /* input */
      {"executable", "Path to user program", ""},
      {"argv","List of arguments for program", ""},
      /* system config */
      DRV_SYS_CONFIG_PARAMETERS
      /* core config */
      {"threads", "Number of threads on this core", "1"},
      {"clock", "Clock rate of core", "125MHz"},
      {"max_idle", "Max idle cycles before we unregister the clock", "1000000"},
      {"id", "ID for the core", "0"},
      {"pod", "Pod ID of this core", "0"},
      {"pxn", "PXN ID of this core", "0"},
      {"phase_max", "Number of preallocated phases for statistic", "1"},
      {"stack_in_l1sp", "Use modeled memory backing store for stack", "0"},
      {"dram_base", "Base address of DRAM", "0x80000000"},
      {"dram_size", "Size of DRAM", "0x100000000"},
      {"l1sp_base", "Base address of L1SP", "0x00000000"},
      {"l1sp_size", "Size of L1SP", "0x00001000"},
      /* debug flags */
      {"verbose", "Verbosity of logging", "0"},
      {"debug_init", "Print debug messages during initialization", "False"},
      {"debug_clock", "Print debug messages we expect to see during clock ticks", "False"},
      {"debug_requests", "Print debug messages we expect to see during request events", "False"},
      {"debug_responses", "Print debug messages we expect to see during response events", "False"},
      {"debug_loopback", "Print debug messages we expect to see during loopback events", "False"},
      {"debug_mmio", "Print debug messages from MMIO write requests", "False"},
      {"trace_remote_pxn", "Trace all requests to remote pxn", "false"},
      {"trace_remote_pxn_load", "Trace loads to remote pxn", "false"},
      {"trace_remote_pxn_store", "Trace loads to remote pxn", "false"},
      {"trace_remote_pxn_atomic", "Trace loads to remote pxn", "false"},
  )
  // Document the ports that this component accepts
  SST_ELI_DOCUMENT_PORTS(
      {"loopback", "A loopback link", {"Drv.DrvEvent", ""}},
  )

  // Document the subcomponents that this component has
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
      {"memory", "Interface to memory hierarchy", "Drv::DrvMemory"},
  )

  struct ThreadStat {
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
      Statistic<uint64_t> *stall_cycles_when_ready;

      Statistic<uint64_t> *tag_cycles; // cycles spent executing with a tag
  };

  static constexpr uint32_t TAG_EXECUTION_LOAD_LEVEL = 3;

  // DOCUMENT STATISTICS
  SST_ELI_DOCUMENT_STATISTICS(
      {"total_load_l1sp", "Number of loads to local L1SP", "count", 1},
      {"total_store_l1sp", "Number of stores to local L1SP", "count", 1},
      {"total_atomic_l1sp", "Number of atomics to local L1SP", "count", 1},
      {"total_load_l2sp", "Number of loads to L2SP", "count", 1},
      {"total_store_l2sp", "Number of stores to L2SP", "count", 1},
      {"total_atomic_l2sp", "Number of atomics to L2SP", "count", 1},
      {"total_load_dram", "Number of loads to DRAM", "count", 1},
      {"total_store_dram", "Number of stores to DRAM", "count", 1},
      {"total_atomic_dram", "Number of atomics to DRAM", "count", 1},
      {"total_load_remote_pxn", "Number of loads to remote PXN", "count", 1},
      {"total_store_remote_pxn", "Number of stores to remote PXN", "count", 1},
      {"total_atomic_remote_pxn", "Number of atomics to remote PXN", "count", 1},
      {"total_stall_cycles_when_ready", "Number of cycles stalled when a thread is ready", "count", 1},
      {"total_tag_cycles", "number of cycles spent executing with a tag", "count", 1},
      {"total_stall_cycles", "Number of stalled cycles", "count", 1},
      {"total_busy_cycles", "Number of busy cycles", "count", 1},
      {"phase_comp_load_l1sp", "Number of loads to local L1SP", "count", 1},
      {"phase_comp_store_l1sp", "Number of stores to local L1SP", "count", 1},
      {"phase_comp_atomic_l1sp", "Number of atomics to local L1SP", "count", 1},
      {"phase_comp_load_l2sp", "Number of loads to L2SP", "count", 1},
      {"phase_comp_store_l2sp", "Number of stores to L2SP", "count", 1},
      {"phase_comp_atomic_l2sp", "Number of atomics to L2SP", "count", 1},
      {"phase_comp_load_dram", "Number of loads to DRAM", "count", 1},
      {"phase_comp_store_dram", "Number of stores to DRAM", "count", 1},
      {"phase_comp_atomic_dram", "Number of atomics to DRAM", "count", 1},
      {"phase_comp_load_remote_pxn", "Number of loads to remote PXN", "count", 1},
      {"phase_comp_store_remote_pxn", "Number of stores to remote PXN", "count", 1},
      {"phase_comp_atomic_remote_pxn", "Number of atomics to remote PXN", "count", 1},
      {"phase_comp_stall_cycles_when_ready", "Number of cycles stalled when a thread is ready", "count", 1},
      {"phase_comp_tag_cycles", "number of cycles spent executing with a tag", "count", 1},
      {"phase_comm_load_l1sp", "Number of loads to local L1SP", "count", 1},
      {"phase_comm_store_l1sp", "Number of stores to local L1SP", "count", 1},
      {"phase_comm_atomic_l1sp", "Number of atomics to local L1SP", "count", 1},
      {"phase_comm_load_l2sp", "Number of loads to L2SP", "count", 1},
      {"phase_comm_store_l2sp", "Number of stores to L2SP", "count", 1},
      {"phase_comm_atomic_l2sp", "Number of atomics to L2SP", "count", 1},
      {"phase_comm_load_dram", "Number of loads to DRAM", "count", 1},
      {"phase_comm_store_dram", "Number of stores to DRAM", "count", 1},
      {"phase_comm_atomic_dram", "Number of atomics to DRAM", "count", 1},
      {"phase_comm_load_remote_pxn", "Number of loads to remote PXN", "count", 1},
      {"phase_comm_store_remote_pxn", "Number of stores to remote PXN", "count", 1},
      {"phase_comm_atomic_remote_pxn", "Number of atomics to remote PXN", "count", 1},
      {"phase_comm_stall_cycles_when_ready", "Number of cycles stalled when a thread is ready", "count", 1},
      {"phase_comm_tag_cycles", "number of cycles spent executing with a tag", "count", 1},
      {"phase_stall_cycles", "Number of stalled cycles", "count", 1},
      {"phase_busy_cycles", "Number of busy cycles", "count", 1},
    )

  /**
   * constructor
   * @param[in] id The component id.
   * @param[in] params Parameters for this component.
   */
  DrvCore(SST::ComponentId_t id, SST::Params& params);

  /** destructor */
  ~DrvCore();

  /**
   * configure the executable
   * @param[in] params Parameters to this component.
   */
  void configureExecutable(SST::Params &params);

  /**
   * close the executable
   */
  void closeExecutable();

  /**
   * configure output logging
   * @param[in] params Parameters to this component.
   */
  void configureOutput(SST::Params &params);

  /**
   * configure output for tracing
   * param[in] params Parameters to this component.
   */
  void configureTrace(SST::Params &params);

  /**
   * configure clock
   * @param[in] params Parameters to this component.
   */
  void configureClock(SST::Params &params);

  /**
   * configure threads on the core
   * @param[in] params  Parameters to this component.
   */
  void configureThreads(SST::Params &params);

  /**
   * configure one thread
   * @param[in] thread A thread to initialize.
   * @param[in[ threads How many threads will be initialized.
   */
  void configureThread(int thread, int threads);

  /**
   * configure memory
   * @param[in] params Parameters to this component.
   */
  void configureMemory(SST::Params &params);

  /**
   * configure other links
   * @param[in] params Parameters to this component.
   */
  void configureOtherLinks(SST::Params &params);

  /**
   * configure sysconfig
   * @param[in] params Parameters to this component.
   */
  void configureSysConfig(SST::Params &params);

  /**
   * configure phase statistics
   */
  void configurePhaseStatistics();

  /**
   * configure statistics
   */
  void configureStatistics();

  /**
   * select a ready thread
   */
  int selectReadyThread();

  /**
   * execute one ready thread
   */
  void executeReadyThread();

  /**
   * parse the command line arguments
   */
  void parseArgv(SST::Params &params);

  /**
   * return true if simulation should finish
   */
  bool allDone();

  /**
   * clock tick handler
   * @param[in] cycle The current cycle number
   */
  virtual bool clockTick(SST::Cycle_t);

  /**
   * handle a loopback event
   * @param[in] event The event to handle
   */
  void handleLoopback(SST::Event *event);

  /**
   * handle a mmio request event
   */
  void handleMMIOWriteRequest(SST::Interfaces::StandardMem::Write* req);

  /**
   * set the current thread context
   */
  void setThreadContext(DrvThread* thread) {
    set_thread_context_(&thread->getAPIThread());
  }

  /**
   * return the output stream of this core
   */
  std::unique_ptr<SST::Output>& output() {
    return output_;
  }

  /**
   * handle a thread state after the the thread has yielded back to the simulator
   * @param[in] thread The thread to handle
   */
  void handleThreadStateAfterYield(DrvThread *thread);

  static constexpr uint32_t DEBUG_INIT     = (1<< 0); //!< debug messages during initialization
  static constexpr uint32_t DEBUG_CLK      = (1<<31); //!< debug messages we expect to see during clock ticks
  static constexpr uint32_t DEBUG_REQ      = (1<<30); //!< debug messages we expect to see when receiving requests
  static constexpr uint32_t DEBUG_RSP      = (1<<29); //!< debug messages we expect to see when receiving responses
  static constexpr uint32_t DEBUG_LOOPBACK = (1<<28); //!< debug messages we expect to see when receiving loopback events
  static constexpr uint32_t DEBUG_MMIO     = (1<<27); //!< debug messages we expect to see when receiving mmio events

  static constexpr uint32_t TRACE_REMOTE_PXN_STORE   = (1<< 0); //!< trace remote store events
  static constexpr uint32_t TRACE_REMOTE_PXN_LOAD    = (1<< 1); //!< trace remote load events
  static constexpr uint32_t TRACE_REMOTE_PXN_ATOMIC  = (1<< 2); //!< trace remote atomic events
  static constexpr uint32_t TRACE_REMOTE_PXN_MONITOR = (1<< 3); //!< trace remote monitor events
  static constexpr uint32_t TRACE_REMOTE_PXN_MEMORY  = (TRACE_REMOTE_PXN_STORE | TRACE_REMOTE_PXN_LOAD | TRACE_REMOTE_PXN_ATOMIC); //!< trace remote memory events
private:
  std::unique_ptr<SST::Output> trace_; //!< for tracing

public:
  void traceRemotePxnMem(uint32_t trace_mask, const char* opname, DrvAPI::DrvAPIPAddress paddr, DrvThread *thread) const {
      trace_->verbose(CALL_INFO, 0, trace_mask
                      ,"OP=%s:SRC_PXN=%d:SRC_POD=%d:SRC_CORE=%d:SRC_THREAD=%d:DST_PXN=%d:ADDR=%s\n"
                      ,opname
                      ,pxn_
                      ,pod_
                      ,id_
                      ,getThreadID(thread)
                      ,(int)paddr.pxn()
                      ,paddr.to_string().c_str()
                      );
  }

  /**
   * initialize the component
   */
  void init(unsigned int phase) override;

  /**
   * setup the component
   */
  void setup() override;

  /**
   * start the threads
   */
  void startThreads();

  /**
   * finish the component
   */
  void finish() override;

  /**
   * update the tag counters
   */
  void updateTagCycles(int times);

  /**
   * return the id of a thread
   */
  int getThreadID(DrvThread *thread) const {
    size_t tid = thread - &threads_[0];
    assert(tid < threads_.size());
    return static_cast<int>(tid);
  }

  /**
   * return pointer to thread
   */
  DrvThread* getThread(int tid) {
    assert(tid >= 0 && static_cast<size_t>(tid) < threads_.size());
    return &threads_[tid];
  }

  /**
   * return the number of threads on this core
   */
  int numThreads() const {
     return static_cast<int>(threads_.size());
  }

  /**
   * return the time converter for the clock
   */
  SST::TimeConverter* getClockTC() {
    return clocktc_;
  }

  /**
   * return true if we should unregister the clock
   */
  bool shouldUnregisterClock() {
    return allDone() || (idle_cycles_ >= max_idle_cycles_);
  }

  /**
   * turn the core on if it's off
   */
  void assertCoreOn() {
    if (!core_on_) {
      core_on_ = true;
      output_->verbose(CALL_INFO, 2, DEBUG_RSP, "turning core on\n");
      reregister_cycle_ = system_callbacks_->getCycleCount();
      addStallCycleStat(reregister_cycle_ - unregister_cycle_);
      updateTagCycles(reregister_cycle_ - unregister_cycle_);
      reregisterClock(clocktc_, clock_handler_);
    }
  }

  /**
   * set the application system configuration
   */
  void setSysConfigApp() {
      DrvAPI::DrvAPISysConfig sys_cfg_app = sys_config_.config();
      set_sys_config_app_(&sys_cfg_app);
  }

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
    void addLoadStat(DrvAPI::DrvAPIPAddress addr, DrvThread *thread) {
        int tid = getThreadID(thread);
        if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMP) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comp_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->load_l1sp->addData(1);
                phase_stats->load_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->load_l2sp->addData(1);
                phase_stats->load_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->load_dram->addData(1);
                phase_stats->load_dram->addData(1);
            } else if (isPAddressRemotePXN(addr))  {
                traceRemotePxnMem(TRACE_REMOTE_PXN_LOAD, "read_req", addr, thread);
                total_stats->load_remote_pxn->addData(1);
                phase_stats->load_remote_pxn->addData(1);
            }
        }
        else if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMM) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comm_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->load_l1sp->addData(1);
                phase_stats->load_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->load_l2sp->addData(1);
                phase_stats->load_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->load_dram->addData(1);
                phase_stats->load_dram->addData(1);
            } else if (isPAddressRemotePXN(addr))  {
                traceRemotePxnMem(TRACE_REMOTE_PXN_LOAD, "read_req", addr, thread);
                total_stats->load_remote_pxn->addData(1);
                phase_stats->load_remote_pxn->addData(1);
            }
        }
    }

    /**
     * add store statistic
     */
    void addStoreStat(DrvAPI::DrvAPIPAddress addr, DrvThread *thread) {
        int tid = getThreadID(thread);
        if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMP) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comp_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->store_l1sp->addData(1);
                phase_stats->store_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->store_l2sp->addData(1);
                phase_stats->store_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->store_dram->addData(1);
                phase_stats->store_dram->addData(1);
            } else if (isPAddressRemotePXN(addr)) {
                traceRemotePxnMem(TRACE_REMOTE_PXN_STORE, "write_req", addr, thread);
                total_stats->store_remote_pxn->addData(1);
                phase_stats->store_remote_pxn->addData(1);
            }
        }
        else if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMM) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comm_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->store_l1sp->addData(1);
                phase_stats->store_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->store_l2sp->addData(1);
                phase_stats->store_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->store_dram->addData(1);
                phase_stats->store_dram->addData(1);
            } else if (isPAddressRemotePXN(addr)) {
                traceRemotePxnMem(TRACE_REMOTE_PXN_STORE, "write_req", addr, thread);
                total_stats->store_remote_pxn->addData(1);
                phase_stats->store_remote_pxn->addData(1);
            }
        }
    }

    /**
     * add atomic statistic
     */
    void addAtomicStat(DrvAPI::DrvAPIPAddress addr, DrvThread *thread) {
        int tid = getThreadID(thread);
        if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMP) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comp_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->atomic_l1sp->addData(1);
                phase_stats->atomic_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->atomic_l2sp->addData(1);
                phase_stats->atomic_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->atomic_dram->addData(1);
                phase_stats->atomic_dram->addData(1);
            } else if (isPAddressRemotePXN(addr))  {
                traceRemotePxnMem(TRACE_REMOTE_PXN_ATOMIC, "atomic_req", addr, thread);
                total_stats->atomic_remote_pxn->addData(1);
                phase_stats->atomic_remote_pxn->addData(1);
            }
        }
        else if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMM) {
            ThreadStat *total_stats = &total_thread_stats_[tid];
            ThreadStat *phase_stats = &(per_phase_comm_thread_stats_[phase_][tid]);
            if (isPAddressL1SP(addr)) {
                total_stats->atomic_l1sp->addData(1);
                phase_stats->atomic_l1sp->addData(1);
            } else if (isPAddressL2SP(addr)) {
                total_stats->atomic_l2sp->addData(1);
                phase_stats->atomic_l2sp->addData(1);
            } else if (isPAddressDRAM(addr)) {
                total_stats->atomic_dram->addData(1);
                phase_stats->atomic_dram->addData(1);
            } else if (isPAddressRemotePXN(addr))  {
                traceRemotePxnMem(TRACE_REMOTE_PXN_ATOMIC, "atomic_req", addr, thread);
                total_stats->atomic_remote_pxn->addData(1);
                phase_stats->atomic_remote_pxn->addData(1);
            }
        }
    }

    void outputStatistics(const std::string &tagname) {
        tag_.verbose(CALL_INFO, 1, 0, "%lu,%s\n", getCurrentSimTime("1 ps"), tagname.c_str());
        performGlobalStatisticOutput();
    }

    void outputPhaseStatistics() {
        output_->verbose(CALL_INFO, 1, DEBUG_CLK, "writing phase statistics\n");
        performStatFileOutput("Dump," + std::to_string(stat_dump_cnt_));
        stat_dump_cnt_++;
        performGlobalStatisticOutput();
    }

    void addBusyCycleStat(uint64_t cycles) {
        if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMP || stage_ == DrvAPI::stage_t::STAGE_EXEC_COMM) {
            total_busy_cycles_->addData(cycles);
            per_phase_busy_cycles_[phase_]->addData(cycles);
        }
    }

    void addStallCycleStat(uint64_t cycles) {
        if (stage_ == DrvAPI::stage_t::STAGE_EXEC_COMP || stage_ == DrvAPI::stage_t::STAGE_EXEC_COMM) {
            total_stall_cycles_->addData(cycles);
            per_phase_stall_cycles_[phase_]->addData(cycles);
        }
    }

    DrvSysConfig &sysConfig() {
        return sys_config_;
    }

    const DrvSysConfig &sysConfig() const {
        return sys_config_;
    }

private:
  std::unique_ptr<SST::Output> output_; //!< for logging
  SST::Output tag_; //!< for stats collection
  std::vector<DrvThread> threads_; //!< the threads on this core
  void *executable_; //!< the executable handle
  drv_api_main_t main_; //!< the main function in the executable
  drv_api_get_thread_context_t get_thread_context_; //!< the get_thread_context function in the executable
  drv_api_set_thread_context_t set_thread_context_; //!< the set_thread_context function in the executable
  DrvAPIGetSysConfig_t get_sys_config_app_; //!< the get_sys_config function in the executable
  DrvAPISetSysConfig_t set_sys_config_app_; //!< the set_sys_config function in the executable
  int done_; //!< number of threads that are done
  int last_thread_; //!< last thread that was executed
  std::vector<char*> argv_; //!< the command line arguments
  SST::Link *loopback_; //!< the loopback link
  uint64_t max_idle_cycles_; //!< maximum number of idle cycles
  uint64_t idle_cycles_; //!< number of idle cycles
  SimTime_t unregister_cycle_; //!< cycle when the clock handler was last unregistered
  SimTime_t reregister_cycle_; //!< cycle when the clock handler was last reregistered
  bool core_on_; //!< true if the core is on (clock handler is registered)
  DrvSysConfig sys_config_; //!< system configuration
  bool stack_in_l1sp_ = false; //!< true if the stack is in L1SP backing store
  std::shared_ptr<DrvSystem> system_callbacks_ = nullptr; //!< the system callbacks
  Clock::Handler<DrvCore> *clock_handler_; //!< the clock handler
  // statistics
  DrvAPI::stage_t stage_; //!< the current stage
  std::vector<ThreadStat> total_thread_stats_; //!< the thread statistics
  std::vector<std::vector<ThreadStat>> per_phase_comp_thread_stats_; //!< the thread statistics
  std::vector<std::vector<ThreadStat>> per_phase_comm_thread_stats_; //!< the thread statistics
  int phase_; //!< the current phase
  Statistic<uint64_t> *total_busy_cycles_; //!< busy cycles
  Statistic<uint64_t> *total_stall_cycles_; //!< stall cycles
  std::vector<Statistic<uint64_t>*> per_phase_busy_cycles_; //!< busy cycles
  std::vector<Statistic<uint64_t>*> per_phase_stall_cycles_; //!< stall cycles
  int stat_dump_cnt_; //!< number of statistics dumps
public:
  DrvMemory* memory_;  //!< the memory hierarchy
  SST::TimeConverter *clocktc_; //!< the clock time converter

  int id_; //!< the core id
  int pod_; //!< pod id of this core
  int pxn_; // !< pxn id of this core
  int num_threads_; //!< number of threads on this core

  int phase_max_; //!< number of preallocated statistics
};
}
}
