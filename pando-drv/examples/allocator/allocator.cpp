// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <inttypes.h>
#include <iostream>
using namespace DrvAPI;

struct foo {
    int a;
    int b;
};
DRV_API_REF_CLASS_BEGIN(foo)
DRV_API_REF_CLASS_DATA_MEMBER(foo, a)
DRV_API_REF_CLASS_DATA_MEMBER(foo, b)
DRV_API_REF_CLASS_END(foo)

DrvAPIGlobalL2SP<int> i;
DrvAPIGlobalL2SP<foo> f;
DrvAPIGlobalL2SP<DrvAPIPointer<int>> pi;

int AllocatorMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    DrvAPIMemoryAllocatorInit();
    for (DrvAPIMemoryType type : {DrvAPIMemoryL1SP, DrvAPIMemoryL2SP, DrvAPIMemoryDRAM}) {
        DrvAPIPointer<int> p0 = DrvAPIMemoryAlloc(type, 0x1000);
        DrvAPIPointer<int> p1 = DrvAPIMemoryAlloc(type, 0x1000);
        std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
        std::cout << "p0 = " << DrvAPIVAddress{p0}.to_string() << std::endl;
        std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
        std::cout << "p1 = " << DrvAPIVAddress{p0}.to_string() << std::endl;
        std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
        std::cout << "p0 = 0x" << std::hex << p0 << std::endl;
        std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
        std::cout << "p1 = 0x" << std::hex << p1 << std::endl;
    }
    foo_ref fref = &f;
    fref.a() = 1;
    fref.b() = 2;
    std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
    std::cout << "&f = 0x" << std::hex << &fref << std::endl;
    std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
    std::cout << "f.a = " << fref.a() << std::endl;
    pi[0] = 1;
    int x = pi[0];
    std::cout << "Core " << myCoreId() << " Thread " << myThreadId() <<":";
    std::cout << "pi[0] = " << x << std::endl;

    return 0;
}
declare_drv_api_main(AllocatorMain);
