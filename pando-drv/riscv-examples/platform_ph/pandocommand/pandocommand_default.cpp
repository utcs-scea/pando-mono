// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#include <pandocommand/executable.hpp>
#include <pandocommand/loader.hpp>
#include <pandocommand/control.hpp>
#include <pandocommand/debug.hpp>
#include <DrvAPI.hpp>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
using namespace DrvAPI;
using namespace pandocommand;

int CommandProcessorMain(int argc, char *argv[])
{
    const char *exe = argv[1];
    PANDOHammerExe executable(exe);
    cmd_dbg("Loading %s\n", exe);
    loadProgram(executable);

    cmd_dbg("Releasing %d Cores on %d Pods from reset\n"
            , numPXNPods()*numPodCores()
            , numPXNPods());

    // release all cores from reset
    assertResetAll(false);
    return 0;
}

declare_drv_api_main(CommandProcessorMain);
