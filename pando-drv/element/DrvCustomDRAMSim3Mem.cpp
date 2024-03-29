// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington

#include <DrvAPIReadModifyWrite.hpp>
#include "DrvCustomStdMem.hpp"
#include "DrvCustomDRAMSim3Mem.hpp"

using namespace SST;
using namespace Drv;
using namespace Interfaces;
using namespace MemHierarchy;

/**
 * constructor
 */
DrvDRAMSim3MemBackend::DrvDRAMSim3MemBackend(ComponentId_t id, Params &params)
    : DRAMSim3Memory(id, params) {
    // get parameters
    int verbose = params.find<int>("verbose_level", 0);
    output_ = SST::Output("[@f:@l:@p] ", verbose, 0, SST::Output::STDOUT);
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/**
 * destructor
 */
DrvDRAMSim3MemBackend::~DrvDRAMSim3MemBackend() {
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/**
 * send a custom request type
 */
bool DrvDRAMSim3MemBackend::issueCustomRequest(ReqId req_id, Interfaces::StandardMem::CustomData *data) {
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
    AtomicReqData *atomic_data = dynamic_cast<AtomicReqData*>(data);
    if (atomic_data) {
        output_.verbose(CALL_INFO, 1, 0, "Received atomic request\n");
        // just model the atomic as a read for now
        Addr addr = atomic_data->pAddr;
        bool ok = memSystem->WillAcceptTransaction(addr, false);
        if (!ok) return false;
        ok = memSystem->AddTransaction(addr, false);
        if (!ok) return false;
        dramReqs[addr].push_back(req_id);
        return true;
    }
    output_.fatal(CALL_INFO, -1, "Error: unknown custom request type\n");
    return false;
}
