// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvMemory.hpp>
#include <DrvCore.hpp>
#include <sstream>
#include <iomanip>

using namespace SST;
using namespace Drv;

DrvMemory::DrvMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core)
    : SubComponent(id)
    , core_(core) {
    // get parameters
    int verbose = params.find<int>("verbose", 0);

    // get debug masks
    uint32_t mask = 0;
    if (params.find<bool>("verbose_init", false))
        mask |= VERBOSE_INIT;
    if (params.find<bool>("verbose_requests", false))
        mask |= VERBOSE_REQ;
    if (params.find<bool>("verbose_responses", false))
        mask |= VERBOSE_RSP;

    // set up output
    std::stringstream ss;
    ss << "[DrvMemory "
       << "{PXN=" << std::setw(2) << core->pxn_
       << ",POD=" << std::setw(2) <<  core->pod_
       << ",CORE=" << std::setw(2) << core->id_
       << "} @t:@f:@l: @p] ";

    output_.init(ss.str(), verbose, mask, SST::Output::STDOUT);
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_INIT, "constructor done\n");
}
