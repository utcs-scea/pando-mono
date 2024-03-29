// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdint>
#include <inttypes.h>
#include <pandocommand/control.hpp>
#include <pandocommand/loader.hpp>

using namespace pandocommand;
using namespace DrvAPI;

int CommandProcessor(int argc, char* argv[]) {
  auto exe = PANDOHammerExe::Open(argv[1]);
  loadProgram(*exe);

  DrvAPIVAddress cp_to_ph_v = CP_TO_PH_ADDR;
  DrvAPIVAddress ph_to_cp_v = PH_TO_CP_ADDR;
  DrvAPI::DrvAPIPointer<int64_t> cp_to_ph = CP_TO_PH_ADDR;
  DrvAPI::DrvAPIPointer<int64_t> ph_to_cp = PH_TO_CP_ADDR;

  *cp_to_ph = 0;
  *ph_to_cp = 0;

  assertResetAll(false);

  printf("CP: cp_to_ph = %s(%" PRIx64 "), ph_to_cp = %s(%" PRIx64 ")\n",
         cp_to_ph_v.to_string().c_str(), cp_to_ph_v.encode(), ph_to_cp_v.to_string().c_str(),
         ph_to_cp_v.encode());
  *cp_to_ph = 1;
  printf("CP: Sent signal to PH\n");
  while (*ph_to_cp == 0)
    ;
  printf("CP: Received signal from PH\n");
  return 0;
}

declare_drv_api_main(CommandProcessor);
