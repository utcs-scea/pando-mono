// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>

using namespace DrvAPI;

uint64_t signal = 0xa5a5a5a5a5a5a5a5;
uint64_t fignal = 0x5a5a5a5a5a5a5a5a;
DrvAPIGlobalL2SP<uint64_t> g_signal;

int CommMain(int argc, char *argv[]) {
    DrvAPIAddress addr = &g_signal;
    if (DrvAPIThread::current()->id() == 0) {
        printf("Thread %2d: writing fignal\n", DrvAPIThread::current()->id());
        write<uint64_t>(addr, fignal);
        printf("Thread %2d: writing signal\n", DrvAPIThread::current()->id());
        write<uint64_t>(addr, signal);
    } else if (DrvAPIThread::current()->id() == 1){
        while (read<uint64_t>(addr) != signal) {
            printf("Thread %2d: waiting for signal\n", DrvAPIThread::current()->id());
        }
        printf("Thread %2d: got signal\n", DrvAPIThread::current()->id());
    } else {
        printf("Thread %2d: not participating\n", DrvAPIThread::current()->id());
    }
    return 0;
}

declare_drv_api_main(CommMain);
