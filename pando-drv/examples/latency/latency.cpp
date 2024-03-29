// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>

int LatencyMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    int arg = 1;
    int cid = 0;
    int tid = 0;
    DrvAPIAddress addr = 0x00000000;
    int n = 100;
    if (arg < argc) {
        cid = atoi(argv[arg++]);
    }
    if (arg < argc) {
        tid = atoi(argv[arg++]);
    }
    if (arg < argc) {
        addr = strtoull(argv[arg++], NULL, 0);
    }
    if (arg < argc) {
        n = atoi(argv[arg++]);
    }

    if (DrvAPIThread::current()->threadId() == tid &&
        DrvAPIThread::current()->coreId() == cid) {
        printf("Latency test from Core %d, Thread %d: N=%d, Address 0x%08lx\n",
               DrvAPIThread::current()->coreId(),
               DrvAPIThread::current()->threadId(),
               n,
               static_cast<unsigned long>(addr));
        // write value to address once
        uint64_t wv = 0x5a5a5a5a5a5a5a5a;
        printf("writing   0x%08lx, w_value=%08" PRIx64 "\n",
               static_cast<unsigned long>(addr), wv);
        DrvAPI::write<uint64_t>(addr, wv);
        // read from address n times
        for (int i = 0; i < n; i++) {
            DrvAPI::read<uint64_t>(addr);
            if (i % 100 == 0) {
                printf("read %4d of %4d\r", i, n);
                outputStatistics("load_" + std::to_string(i));
            }
        }
        outputStatistics("done");
        printf("\ndone!\n");
    }
    return 0;
}
declare_drv_api_main(LatencyMain);
