// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_OP_H
#define DRV_API_OP_H
#include <DrvAPIThread.hpp>
#include <DrvAPIThreadState.hpp>
namespace DrvAPI
{

inline void nop(int cycles)
{
    DrvAPIThread::current()->setState
        (std::make_shared<DrvAPINop>(cycles));
    DrvAPIThread::current()->yield();
}

inline void wait(int cycles)
{
    return nop(cycles);
}

}
#endif
