// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>

int AddrMapMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    uint32_t value = 0;
    auto _19_16 = bits::bitrange_handle<uint32_t, 19, 16>(value);
    _19_16 = 0xa;

    auto _23_20 = bits::bitrange_handle<uint32_t, 23, 20>(value);
    _23_20 = _19_16;

    printf("value = 0x%08x\n", value);


    DrvAPIAddress addr = 0;
    DrvAPIVAddress vaddr(addr);
    vaddr.pxn() = -1;
    vaddr.global() = true;
    vaddr.pod() = 7;

    DrvAPIVAddress vaddr2 = 0;

    vaddr2.dram_offset_lo33() = +8ull*1024ull*1024ull*1024ull-1ull;
    vaddr2.dram_offset_hi10() = 3;
    vaddr2.not_scratchpad() = 1;;

    auto vaddr3 = vaddr2;
    vaddr3.not_scratchpad() = 0;

    printf("vaddr  = %s (%016" PRIx64 ")\n", vaddr.to_string().c_str(), vaddr.encode());
    printf("vaddr2 = %s (%016" PRIx64 ")\n", vaddr2.to_string().c_str(), vaddr2.encode());
    printf("vaddr3 = %s (%016" PRIx64 ")\n", vaddr3.to_string().c_str(), vaddr3.encode());

    auto paddr = vaddr.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());
    auto paddr2 = vaddr2.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());
    auto paddr3 = vaddr3.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());

    printf("paddr  = %s (%016" PRIx64 ")\n", paddr.to_string().c_str(), paddr.encode());
    printf("paddr2 = %s (%016" PRIx64 ")\n", paddr2.to_string().c_str(), paddr2.encode());
    printf("paddr3 = %s (%016" PRIx64 ")\n", paddr3.to_string().c_str(), paddr3.encode());

    auto l2spbase = DrvAPIVAddress::MyL2Base();
    auto l2sbase_global = l2spbase;
    l2sbase_global.global() = true;

    printf("l2spbase       = %s (%016" PRIx64 ")\n", l2spbase.to_string().c_str(), l2spbase.encode());
    printf("l2sbase_global = %s (%016" PRIx64 ")\n", l2sbase_global.to_string().c_str(), l2sbase_global.encode());

    auto l2spbase_phys = l2spbase.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());
    auto l2sbase_global_phys = l2sbase_global.to_physical(myPXNId(), myPodId(), myCoreY(), myCoreX());
    printf("l2spbase_phys       = %s (%016" PRIx64 ")\n", l2spbase_phys.to_string().c_str(), l2spbase_phys.encode());
    printf("l2sbase_global_phys = %s (%016" PRIx64 ")\n", l2sbase_global_phys.to_string().c_str(), l2sbase_global_phys.encode());
    return 0;
}

declare_drv_api_main(AddrMapMain);
