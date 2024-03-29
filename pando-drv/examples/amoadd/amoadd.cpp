// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>

DrvAPI::DrvAPIGlobalL2SP<int64_t> signal_var;
DrvAPI::DrvAPIGlobalL2SP<int64_t> barrier_var;

int AmoaddMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    DrvAPIAddress signal_addr  = &signal_var;
    DrvAPIAddress barrier_addr = &barrier_var;
    if (argc > 1) {
        barrier_addr = strtoull(argv[1], NULL, 0);
        if (argc > 2) {
            signal_addr = strtoull(argv[2], NULL, 0);
        }
    }
    printf("Hello from %s\n", __PRETTY_FUNCTION__);
    printf("barrier_addr = %lx, signal_addr = %lx\n",
           static_cast<unsigned long>(barrier_addr),
           static_cast<unsigned long>(signal_addr));
    int64_t signal = 0xa5a5a5a5a5a5a5a5;

    if (DrvAPIThread::current()->threadId() == 0 &&
        DrvAPIThread::current()->coreId() == 0 ) {
        DrvAPI::write<int64_t>(barrier_addr, 0);
        DrvAPI::write<int64_t>(signal_addr, signal);
        printf("Thread %2d: Core %2d: writing signal\n",
               DrvAPIThread::current()->threadId(),
               DrvAPIThread::current()->coreId());
    }

    printf("Thread %2d: Core %2d: waiting for signal\n",
           DrvAPIThread::current()->threadId(),
           DrvAPIThread::current()->coreId());

    while (DrvAPI::read<int64_t>(signal_addr) != signal);

    printf("Thread %2d: Core %2d: got signal; adding 1 to barrier\n",
           DrvAPIThread::current()->threadId(),
           DrvAPIThread::current()->coreId());

    int64_t b = DrvAPI::atomic_add<int64_t>(barrier_addr, 1);
    printf("Thread %2d: Core %2d: read %ld after adding to barrier\n",
           DrvAPIThread::current()->threadId(),
           DrvAPIThread::current()->coreId(),
           b);

    return 0;
}

declare_drv_api_main(AmoaddMain);
