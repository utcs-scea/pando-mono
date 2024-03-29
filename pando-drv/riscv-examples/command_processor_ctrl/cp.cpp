// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdint>
#include <inttypes.h>
#include <pandocommand/control.hpp>
#include <pandocommand/loader.hpp>
using namespace DrvAPI;
using namespace pandocommand;

int CommandProcessor(int argc, char* argv[]) {
  DrvAPIVAddress cp_to_ph_v = CP_TO_PH_ADDR;
  DrvAPIVAddress ph_to_cp_v = PH_TO_CP_ADDR;
  DrvAPI::DrvAPIPointer<int64_t> cp_to_ph = CP_TO_PH_ADDR;
  DrvAPI::DrvAPIPointer<int64_t> ph_to_cp = PH_TO_CP_ADDR;

  auto exe = PANDOHammerExe::Open(argv[1]);
  loadProgram(*exe);

  // initialize sync variables
  *cp_to_ph = 0;
  *ph_to_cp = 0;

  assertResetAll(false);

  *cp_to_ph = KEY;
  printf("CP: Sent signal to PH\n");
  while (*ph_to_cp < THREADS * CORES) {
    DrvAPI::wait(1000);
  }
  printf("CP: Received signal from %ld PH Threads\n", (int64_t)*ph_to_cp);
  return 0;
}

declare_drv_api_main(CommandProcessor);
