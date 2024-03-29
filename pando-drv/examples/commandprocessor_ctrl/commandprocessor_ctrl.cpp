// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <string>
#include <cstdio>
#include <inttypes.h>
using namespace DrvAPI;

DrvAPIGlobalDRAM<int64_t> done;

int CPMain(int argc, char *argv[])
{
    if (!isCommandProcessor()) {
        while (!done) {
            DrvAPI::wait(1000);
        }
    } else {
        // ctrl registers for pxn 0, pod 0, core 0
        DrvAPIVAddress ctrl_v = DrvAPIVAddress::CoreCtrlBase(0, 0, 0, 0);
        DrvAPIPAddress ctrl_p = ctrl_v.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());
        printf("ctrl_v = %s\n", ctrl_v.to_string().c_str());
        printf("ctrl_p = %s\n", ctrl_p.to_string().c_str());
        DrvAPI::write(ctrl_v.encode(), 0xdeadbeef);
    }
    done = 1;
    return 0;
}

declare_drv_api_main(CPMain);
