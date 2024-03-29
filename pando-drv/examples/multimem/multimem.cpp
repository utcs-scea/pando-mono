// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <vector>
#include <cstdio>
#include <inttypes.h>
int MultiMemMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    int cid = 0;
    int tid = 0;
    int arg = 1;
    if (arg < argc) {
        cid = atoi(argv[arg++]);
    }
    if (arg < argc) {
        tid = atoi(argv[arg++]);
    }
    if (DrvAPIThread::current()->coreId() == cid &&
        DrvAPIThread::current()->threadId() == tid) {
        printf("Hello from Core %d, Thread %d\n"
               ,DrvAPIThread::current()->coreId()
               ,DrvAPIThread::current()->threadId());

        std::vector<DrvAPIAddress> addrs;
        while (arg < argc) {
            DrvAPIAddress addr = strtoull(argv[arg++], NULL, 0);
            printf("parsed    0x%08lx\n", static_cast<unsigned long>(addr));
            addrs.push_back(addr);
        }
        uint64_t writeval = 0x5a5a5a5a5a5a5a5a;
        // write and read-back
        for (DrvAPIAddress addr : addrs) {
            writeval = ~writeval;
            uint64_t swapval  = ~writeval;
            printf("writing   0x%08lx, w_value=%08" PRIx64 "\n",
                    static_cast<unsigned long>(addr), writeval);
            DrvAPI::write<uint64_t>(addr, writeval);
            uint64_t readback = DrvAPI::read<uint64_t>(addr);
            printf("reading   0x%08lx, r_value=%08" PRIx64 "\n",
                   static_cast<unsigned long>(addr), readback);
            readback = DrvAPI::atomic_swap<uint64_t>(addr, swapval);
            printf("swapping  0x%08lx, w_value=%08" PRIx64 ", r_value %08" PRIx64 "\n",
                   static_cast<unsigned long>(addr), swapval, readback);
            readback = DrvAPI::atomic_swap<uint64_t>(addr, writeval);
            printf("re-swap   0x%08lx, w_value=%08" PRIx64 ", r_value %08" PRIx64 "\n",
                   static_cast<unsigned long>(addr), writeval, readback);
        }
        printf("done!\n");
    }
    return 0;
}


declare_drv_api_main(MultiMemMain);
