// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPIGlobal.hpp>
#include <DrvAPIAllocator.hpp>
#include <DrvAPIAddress.hpp>
#include <DrvAPIAddressMap.hpp>
#include "DrvCore.hpp"
#include "DrvSimpleMemory.hpp"
#include "DrvSelfLinkMemory.hpp"
#include "DrvStdMemory.hpp"
#include "DrvNopEvent.hpp"
#include <cstdio>
#include <dlfcn.h>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace SST;
using namespace Drv;

//////////////////////////
// init and finish code //
//////////////////////////
/**
 * configure the output logging
 */
void DrvCore::configureOutput(SST::Params &params) {
  int verbose_level = params.find<int>("verbose");
  uint32_t verbose_mask = 0;
  if (params.find<bool>("debug_init"))
    verbose_mask |= DEBUG_INIT;
  if (params.find<bool>("debug_clock"))
    verbose_mask |= DEBUG_CLK;
  if (params.find<bool>("debug_requests"))
    verbose_mask |= DEBUG_REQ;
  if (params.find<bool>("debug_responses"))
    verbose_mask |= DEBUG_RSP;
  if (params.find<bool>("debug_mmio"))
    verbose_mask |= DEBUG_MMIO;
  std::stringstream ss;
  ss << "[DrvCore "
     << "{PXN=" << std::setw(2) << pxn_
     << ",POD=" << std::setw(2) <<  pod_
     << ",CORE=" << std::setw(2) << id_
     << "} @t: @f:@l: @p] ";
  output_ = std::make_unique<SST::Output>(ss.str(), verbose_level, verbose_mask, Output::STDOUT);
  output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configured output logging\n");
}

void DrvCore::configureTrace(SST::Params &params) {
    uint32_t trace_mask = 0;
    if (params.find<bool>("trace_remote_pxn"))
        trace_mask |= TRACE_REMOTE_PXN_MEMORY;
    if (params.find<bool>("trace_remote_pxn_load"))
        trace_mask |= TRACE_REMOTE_PXN_LOAD;
    if (params.find<bool>("trace_remote_pxn_store"))
        trace_mask |= TRACE_REMOTE_PXN_STORE;
    if (params.find<bool>("trace_remote_pxn_atomic"))
        trace_mask |= TRACE_REMOTE_PXN_ATOMIC;
    trace_ = std::make_unique<SST::Output>("@t:", 0, trace_mask, Output::FILE);
}

/**
 * configure the executable */
void DrvCore::configureExecutable(SST::Params &params) {
  std::string executable = params.find<std::string>("executable");
  argv_.push_back(strdup(executable.c_str()));
  if (executable.empty()) {
    output_->fatal(CALL_INFO, -1, "executable not specified\n");
  }

  output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configuring executable: %s\n", executable.c_str());
  executable_ = dlopen(executable.c_str(), RTLD_LAZY|RTLD_LOCAL);
  if (!executable_) {
    output_->fatal(CALL_INFO, -1, "unable to open executable: %s\n", dlerror());
  }

  main_ = (drv_api_main_t)dlsym(executable_, "__drv_api_main");
  if (!main_) {
    output_->fatal(CALL_INFO, -1, "unable to find __drv_api_main in executable: %s\n", dlerror());
  }

  get_thread_context_ = (drv_api_get_thread_context_t)dlsym(executable_, "DrvAPIGetCurrentContext");
  if (!get_thread_context_) {
    output_->fatal(CALL_INFO, -1, "unable to find DrvAPIGetCurrentContext in executable: %s\n", dlerror());
  }

  set_thread_context_ = (drv_api_set_thread_context_t)dlsym(executable_, "DrvAPISetCurrentContext");
  if (!set_thread_context_) {
    output_->fatal(CALL_INFO, -1, "unable to find DrvAPISetCurrentContext in executable: %s\n", dlerror());
  }

  get_sys_config_app_ = (DrvAPIGetSysConfig_t)dlsym(executable_, "DrvAPIGetSysConfig");
  if (!get_sys_config_app_) {
      output_->fatal(CALL_INFO, -1, "unable to find DrvAPIGetSysConfig in executable: %s\n", dlerror());
  }

  set_sys_config_app_ = (DrvAPISetSysConfig_t)dlsym(executable_, "DrvAPISetSysConfig");
  if (!set_sys_config_app_) {
      output_->fatal(CALL_INFO, -1, "unable to find DrvAPISetSysConfig in executable: %s\n", dlerror());
  }

  output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configured executable\n");
}

/**
 * close the executable
 */
void DrvCore::closeExecutable() {
  if (dlclose(executable_)) {
    output_->fatal(CALL_INFO, -1, "unable to close executable: %s\n", dlerror());
  }
  executable_ = nullptr;
}

/**
 * configure the clock and register the handler
 */
void DrvCore::configureClock(SST::Params &params) {
  clock_handler_ = new Clock::Handler<DrvCore>(this, &DrvCore::clockTick);
  clocktc_ = registerClock(params.find<std::string>("clock", "125MHz"), clock_handler_);
  max_idle_cycles_ = params.find<uint64_t>("max_idle", 1000000);
  core_on_ = true;
}

/**
 * configure one thread
 */
void DrvCore::configureThread(int thread, int threads) {
  output_->verbose(CALL_INFO, 2, DEBUG_INIT, "configuring thread (%2d/%2d)\n", thread, threads);
  threads_.emplace_back();
  DrvAPI::DrvAPIThread& api_thread = threads_.back().getAPIThread();
  api_thread.setMain(main_);
  api_thread.setArgs(argv_.size(), argv_.data());
  api_thread.setId(thread);
  api_thread.setCoreId(id_);
  api_thread.setCoreThreads(threads);
  api_thread.setPodId(pod_);
  api_thread.setPxnId(pxn_);
  api_thread.setStackInL1SP(stack_in_l1sp_);
  api_thread.setSystem(system_callbacks_);
}

/**
 * configure threads on the core
 */
void DrvCore::configureThreads(SST::Params &params) {
  int threads = params.find<int>("threads", 1);
  stack_in_l1sp_ = params.find<bool>("stack_in_l1sp", false);
  output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configuring %d threads\n", threads);
  for (int thread = 0; thread < threads; thread++)
    configureThread(thread, threads);
  done_ = threads;
  last_thread_ = threads - 1;
}

void DrvCore::startThreads() {
    for (auto& thread : threads_) {
        thread.getAPIThread().start();
    }
}

/**
 * configure the memory
 */
void DrvCore::configureMemory(SST::Params &params) {
    memory_ = loadUserSubComponent<DrvMemory>("memory", ComponentInfo::SHARE_NONE, this);
    if (!memory_) {
        output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configuring simple memory\n");
        SST::Params mem_params = params.get_scoped_params("memory");
        memory_ = loadAnonymousSubComponent<DrvMemory>
            ("Drv.DrvSimpleMemory", "memory", 0, ComponentInfo::SHARE_NONE, mem_params, this);
        if (!memory_) {
            output_->fatal(CALL_INFO, -1, "unable to load memory subcomponent\n");
        }
    }
    DrvAPI::DrvAPIAddress dram_base_default
        = DrvAPI::DrvAPIVAddress::MainMemBase(pxn_).encode();
    DrvAPI::DrvAPISection::GetSection(DrvAPI::DrvAPIMemoryDRAM)
        .setBase(dram_base_default, pxn_, pod_, id_);

    // default l2 statics to base of local l2 scratchpad
    DrvAPI::DrvAPIAddress l2sp_base_default = DrvAPI::DrvAPIVAddress::MyL2Base().encode();
    uint64_t l2sp_base = params.find<uint64_t>("l2sp_base", l2sp_base_default);
    DrvAPI::DrvAPISection::GetSection(DrvAPI::DrvAPIMemoryL2SP)
        .setBase(l2sp_base, pxn_, pod_, id_);


    // default l1 statics to base of local l1 scratchpad
    DrvAPI::DrvAPIAddress l1sp_base_default = DrvAPI::DrvAPIVAddress::MyL1Base().encode();
    uint64_t l1sp_base = params.find<uint64_t>("l1sp_base", l1sp_base_default);
    DrvAPI::DrvAPISection::GetSection(DrvAPI::DrvAPIMemoryL1SP)
        .setBase(l1sp_base, pxn_, pod_, id_);
}

/*
 * configure the other links
 */
void DrvCore::configureOtherLinks(SST::Params &params) {
    loopback_ = configureSelfLink("loopback", new Event::Handler<DrvCore>(this, &DrvCore::handleLoopback));
    loopback_->addSendLatency(1, "ns");
}

/**
 * configure the statistics
 */
void DrvCore::configureStatistics(Params &params) {
    output_->verbose(CALL_INFO, 1, DEBUG_INIT, "configuring statistics\n");
    uint32_t stats_level = getStatisticLoadLevel();
    tag_.init("", stats_level, 0, Output::FILE, "tags.csv");
    tag_.verbose(CALL_INFO, 1, 0, "SimTime,TagName\n");

    int threads = params.find<int>("threads", 1);
    thread_stats_.resize(threads);
    for (int thread = 0; thread < threads; thread++) {
        ThreadStat *stat = &thread_stats_[thread];
        std::string subid = "thread_" + std::to_string(thread);
        stat->load_l1sp = registerStatistic<uint64_t>("load_l1sp", subid);
        stat->load_l2sp = registerStatistic<uint64_t>("load_l2sp", subid);
        stat->load_dram = registerStatistic<uint64_t>("load_dram", subid);
        stat->load_remote_pxn = registerStatistic<uint64_t>("load_remote_pxn", subid);
        stat->store_l1sp = registerStatistic<uint64_t>("store_l1sp", subid);
        stat->store_l2sp = registerStatistic<uint64_t>("store_l2sp", subid);
        stat->store_dram = registerStatistic<uint64_t>("store_dram", subid);
        stat->store_remote_pxn = registerStatistic<uint64_t>("store_remote_pxn", subid);
        stat->atomic_l1sp = registerStatistic<uint64_t>("atomic_l1sp", subid);
        stat->atomic_l2sp = registerStatistic<uint64_t>("atomic_l2sp", subid);
        stat->atomic_dram = registerStatistic<uint64_t>("atomic_dram", subid);
        stat->atomic_remote_pxn = registerStatistic<uint64_t>("atomic_remote_pxn", subid);
        stat->tag_cycles = registerStatistic<uint64_t>("tag_cycles", subid);
    }
    busy_cycles_ = registerStatistic<uint64_t>("busy_cycles");
    stall_cycles_ = registerStatistic<uint64_t>("stall_cycles");
}

/**
 * configure sysconfig
 * @param[in] params Parameters to this component.
 */
void DrvCore::configureSysConfig(SST::Params &params) {
    sys_config_.init(params);
    DrvAPI::DrvAPISysConfig cfg = sys_config_.config();
    output_->verbose(CALL_INFO, 1, DEBUG_INIT,
                     "configured sysconfig: "
                     "num_pxn = %" PRId64 ", "
                     "pxn_pods = %" PRId64 ", "
                     "pod_cores = %" PRId64 ", "
                     "core_threads = %d"
                     "\n"
                     ,cfg.numPXN()
                     ,cfg.numPXNPods()
                     ,cfg.numPodCores()
                     ,numThreads()
                     );
}

void DrvCore::parseArgv(SST::Params &params) {
    std::string argv_str = params.find<std::string>("argv", "");
    std::stringstream ss(argv_str);
    while (!ss.eof()) {
        std::string arg;
        ss >> arg;
        if (!arg.empty()) {
            argv_.push_back(strdup(arg.c_str()));
        }
    }
}

DrvCore::DrvCore(SST::ComponentId_t id, SST::Params& params)
  : SST::Component(id)
  , executable_(nullptr)
  , loopback_(nullptr)
  , idle_cycles_(0)
  , core_on_(false)
  , system_callbacks_(std::make_shared<DrvSystem>(*this)) {
  id_ = params.find<int>("id", 0);
  pod_ = params.find<int>("pod", 0);
  pxn_ = params.find<int>("pxn", 0);
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
  configureOutput(params);
  configureTrace(params);
  configureSysConfig(params);
  configureClock(params);
  configureMemory(params);
  configureOtherLinks(params);
  configureExecutable(params);
  configureStatistics(params);
  parseArgv(params);
  configureThreads(params);
  setSysConfigApp();
}

DrvCore::~DrvCore() {
    // the last thing we should is close the executable
    // this keeps the vtable entries valid for dynamic classes
    // created in the user code
    closeExecutable();
    // clean up the argv
    for (auto arg : argv_) {
        free(arg);
    }
    delete memory_;
}

///////////////////////////
// SST simulation phases //
///////////////////////////
/**
 * initialize the component
 */
void DrvCore::init(unsigned int phase) {
  auto stdmem = dynamic_cast<DrvStdMemory*>(memory_);
  if (stdmem) {
    stdmem->init(phase);
  }
}

/**
 * finish the component
 */
void DrvCore::setup() {
  auto stdmem = dynamic_cast<DrvStdMemory*>(memory_);
  if (stdmem) {
    stdmem->setup();
  }
  startThreads();
}

/**
 * finish the component
 */
void DrvCore::finish() {
  Cycle_t cycle = getNextClockCycle(clocktc_);
  cycle--;
  updateTagCycles(cycle-unregister_cycle_);
  addStallCycleStat(cycle-unregister_cycle_);
  threads_.clear();
  auto stdmem = dynamic_cast<DrvStdMemory*>(memory_);
  if (stdmem) {
    stdmem->finish();
  }
}

/////////////////////
// clock tick code //
/////////////////////
static constexpr int NO_THREAD_READY = -1;

int DrvCore::selectReadyThread() {
  // select a ready thread to execute
  for (int t = 0; t < numThreads(); t++) {
    int thread_id = (last_thread_ + t + 1) % numThreads();
    DrvThread *thread = getThread(thread_id);
    auto & state = thread->getAPIThread().getState();
    if (state->canResume()) {
      output_->verbose(CALL_INFO, 2, DEBUG_CLK, "thread %d is ready\n", thread_id);
      return thread_id;
    }
  }
  output_->verbose(CALL_INFO, 2, DEBUG_CLK, "no thread is ready\n");
  return NO_THREAD_READY;
}

void DrvCore::updateReadyThreadStallCycleStat(int selected_thread_id) {
  // update the stall cycles for the threads that are ready but not selected
  if (selected_thread_id != NO_THREAD_READY) {
    for (int t = 0; t < numThreads(); t++) {
      if (t == selected_thread_id) {
        continue;
      }
      else {
        DrvThread *thread = getThread(t);
        auto & state = thread->getAPIThread().getState();
        if (state->canResume()) {
          addReadyThreadStallCycleStat(1, thread);
        }
      }
    }
  }
}

void DrvCore::executeReadyThread() {
  // select a ready thread to execute
  int thread_id = selectReadyThread();
  updateReadyThreadStallCycleStat(thread_id);
  if (thread_id == NO_THREAD_READY) {
    addStallCycleStat(1);
    idle_cycles_++;
    return;
  }
  idle_cycles_ = 0;

  // execute the ready thread
  threads_[thread_id].execute(this);
  last_thread_ = thread_id;

  addBusyCycleStat(1);
}

void DrvCore::handleThreadStateAfterYield(DrvThread *thread) {
  std::shared_ptr<DrvAPI::DrvAPIThreadState> &state = thread->getAPIThread().getState();
  std::shared_ptr<DrvAPI::DrvAPIMem> mem_req = std::dynamic_pointer_cast<DrvAPI::DrvAPIMem>(state);

  // handle memory requests
  if (mem_req) {
    // for now; do nothing
    memory_->sendRequest(this, thread, mem_req);
    return;
  }

  // handle nop
  std::shared_ptr<DrvAPI::DrvAPINop> nop_req = std::dynamic_pointer_cast<DrvAPI::DrvAPINop>(state);
  if (nop_req) {
    output_->verbose(CALL_INFO, 1, DEBUG_CLK, "thread %d nop for %d cycles\n", getThreadID(thread), nop_req->count());
    //TimeConverter *tc = getTimeConverter("1ns");
    loopback_->send(nop_req->count(), clocktc_, new DrvNopEvent(getThreadID(thread)));
    return;
  }

  // handle termination
  std::shared_ptr<DrvAPI::DrvAPITerminate> term_req = std::dynamic_pointer_cast<DrvAPI::DrvAPITerminate>(state);
  if (term_req) {
    output_->verbose(CALL_INFO, 1, DEBUG_CLK, "thread %d terminated\n", getThreadID(thread));
    done_--;
    return;
  }

  // fatal - unknown state
  output_->fatal(CALL_INFO, -1, "unknown thread state\n");
  return;
}

bool DrvCore::allDone() {
  return done_ == 0;
}

void DrvCore::updateTagCycles(int times) {
    // skip if these are null stats
    if (getStatisticLoadLevel() < TAG_EXECUTION_LOAD_LEVEL) {
        return;
    }

    for (auto &drv_thread : threads_) {
        int tid = getThreadID(&drv_thread);
        auto &thread_stats = thread_stats_[tid];
        thread_stats.tag_cycles->addDataNTimes(times, drv_thread.getAPIThread().getTag());
    }
}

bool DrvCore::clockTick(SST::Cycle_t cycle) {
  output_->verbose(CALL_INFO, 20, DEBUG_CLK, "tick!\n");
  // execute a ready thread
  executeReadyThread();
  if (allDone()) {
      primaryComponentOKToEndSim();
  }
  updateTagCycles(1);
  bool unregister = shouldUnregisterClock();
  core_on_ = !unregister;
  if (unregister) {
    output_->verbose(CALL_INFO, 2, DEBUG_CLK, "unregistering clock\n");
    // save the time for statistics
    unregister_cycle_ = system_callbacks_->getCycleCount();
  }
  return unregister;
}

void DrvCore::handleLoopback(SST::Event *event) {
  DrvEvent *drv_event = dynamic_cast<DrvEvent*>(event);
  if (!drv_event) {
    output_->fatal(CALL_INFO, -1, "loopback event is not a thread\n");
  }
  // nop events
  DrvNopEvent *nop_event = dynamic_cast<DrvNopEvent*>(drv_event);
  if (nop_event) {
    output_->verbose(CALL_INFO, 2, DEBUG_LOOPBACK, "loopback event is a nop\n");
    DrvThread *thread = getThread(nop_event->tid);
    auto nop = std::dynamic_pointer_cast<DrvAPI::DrvAPINop>(thread->getAPIThread().getState());
    if (!nop) {
      output_->fatal(CALL_INFO, -1, "loopback event is not a nop\n");
    }
    nop->complete();
    assertCoreOn();
    delete event;
    return;
  }
}

/**
 * handle a memory request to control registers
 * the request is deleted by the caller
 */
void DrvCore::handleMMIOWriteRequest(Interfaces::StandardMem::Write* req) {
    output_->verbose(CALL_INFO, 0, DEBUG_MMIO,
                     "PXN %d: POD %d: Core %d: handling mmio write request\n"
                     ,pxn_,pod_,id_);
}
