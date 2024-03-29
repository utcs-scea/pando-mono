// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>

int ThreadMain(int argc, char *argv[]) {
    printf("Hello from thread %2d (%p)\n"
           ,DrvAPI::DrvAPIThread::current()->id()
           ,static_cast<void*>(DrvAPI::DrvAPIThread::current()));
    return 0;
}

declare_drv_api_main(ThreadMain);
