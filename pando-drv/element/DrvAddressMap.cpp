// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAddressMap.hpp>

using namespace SST;
using namespace Drv;
using namespace DrvAPI;

DrvAddressMap::DrvAddressMap(SST::ComponentId_t id, SST::Params& params) : SubComponent(id) {
  // get parameters
  bool verbose = params.find<bool>("verbose", false);

  // set up output
  output_.init("[DrvAddressMap @t:@f:@l: @p]", verbose, 0, SST::Output::STDOUT);
  output_.verbose(CALL_INFO, 1, 0, "done\n");
}

DrvAddressMap::~DrvAddressMap() {}

SST::Interfaces::StandardMem::Addr DrvAddressMap::addrVirtualToPhysical(
    const DrvAPIAddress& virt) const {
  return static_cast<Interfaces::StandardMem::Addr>(virt);
}
