// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPIReadModifyWrite.hpp>
#include "DrvCustomStdMem.hpp"
#include "DrvCustomRamulatorMem.hpp"
#include "Request.h" // ramulator

using namespace SST;
using namespace Drv;
using namespace Interfaces;
using namespace MemHierarchy;

/**
 * constructor of our ramulator backend
 */
DrvRamulatorMemBackend::DrvRamulatorMemBackend(ComponentId_t id, Params &params)
    : ramulatorMemory(id, params) {
    int verbose_level = params.find<int>("verbose_level", 0);
    output_ = SST::Output("[@f:@l:@p]: ", verbose_level, 0, SST::Output::STDOUT);
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/**
 * destructor
 */
DrvRamulatorMemBackend::~DrvRamulatorMemBackend() {
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/**
 * handle custom requests for drv componenets
 */
bool DrvRamulatorMemBackend::issueCustomRequest(ReqId req_id, Interfaces::StandardMem::CustomData *data) {
    output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
    AtomicReqData *atomic_data = dynamic_cast<AtomicReqData*>(data);
    if (atomic_data) {
        output_.verbose(CALL_INFO, 1, 0, "Received atomic request\n");
        // just model the atomic as a read for now
        Addr addr = atomic_data->pAddr;
        ramulator::Request request
            (addr
             ,ramulator::Request::Type::READ
             ,callBackFunc
             ,0 // context or core id
             );
        bool ok = memSystem->send(request);
        if (!ok) return false;
        dramReqs[addr].push_back(req_id);
        return true;
    }
    output_.fatal(CALL_INFO, -1, "Error: unknown custom request type\n");
    return false;
}
