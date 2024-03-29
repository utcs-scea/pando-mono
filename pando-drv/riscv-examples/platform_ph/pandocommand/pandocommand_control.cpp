// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#include <DrvAPI.hpp>
#include <pandocommand/control.hpp>
using namespace DrvAPI;
using namespace pandocommand;

namespace pandocommand {

void assertResetAll(bool reset) {
  // release all cores from reset
  for (int pod = 0; pod < numPXNPods(); pod++) {
    for (int core = 0; core < numPodCores(); core++) {
      DrvAPIVAddress reset_addr =
          DrvAPIVAddress::CoreCtrlBase(myPXNId(), pod, coreYFromId(core), coreXFromId(core));
      write(reset_addr.encode(), reset ? 1l : 0l);
    }
  }
}

} // namespace pandocommand
