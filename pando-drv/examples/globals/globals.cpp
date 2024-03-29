// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>

#define pr_info(fmt, ...)                                               \
    do {                                                                \
        printf("PXN %3d: POD: %3d: CORE %3d: " fmt ""                   \
               ,myPXNId()                                               \
               ,myPodId()                                               \
               ,myCoreId()                                              \
               ,##__VA_ARGS__);                                         \
    } while (0)

using namespace DrvAPI;

DrvAPIGlobalL1SP<int> g_l1sp;
DrvAPIGlobalL2SP<int> g_l2sp;
DrvAPIGlobalDRAM<int> g_dram;

DrvAPIPAddress to_physical(DrvAPIAddress addr)
{
    return DrvAPIVAddress::to_physical(addr, myPXNId(), myPodId(), myCoreY(), myCoreX());
}

int GlobalsMain(int argc, char *argv[])
{
    DrvAPIVAddress g_l1sp_base = DrvAPIVAddress::MyL1Base();
    DrvAPIVAddress g_l2sp_base = DrvAPIVAddress::MyL2Base();
    DrvAPIVAddress g_dram_base = DrvAPIVAddress::MainMemBase(myPXNId());

    pr_info("g_l1sp_base = %016" PRIx64 "\n", g_l1sp_base.encode());
    pr_info("g_l2sp_base = %016" PRIx64 "\n", g_l2sp_base.encode());
    pr_info("g_dram_base = %016" PRIx64 "\n", g_dram_base.encode());

    pr_info("&g_l1sp     = %016" PRIx64 "\n", (DrvAPIAddress)&g_l1sp);
    pr_info("&g_l2sp     = %016" PRIx64 "\n", (DrvAPIAddress)&g_l2sp);
    pr_info("&g_dram     = %016" PRIx64 "\n", (DrvAPIAddress)&g_dram);

    pr_info("&g_l1sp:      %s\n", to_physical(&g_l1sp).to_string().c_str());
    pr_info("&g_l2sp:      %s\n", to_physical(&g_l2sp).to_string().c_str());
    pr_info("&g_dram:      %s\n", to_physical(&g_dram).to_string().c_str());

    if (!DrvAPIVAddress(&g_l1sp).is_l1()) {
        pr_info("ERROR: g_l1sp is not in L1\n");
        return 1;
    }
    if (!DrvAPIVAddress(&g_l2sp).is_l2()) {
        pr_info("g_l2sp is not in L2\n");
        return 1;
    }
    if (!DrvAPIVAddress(&g_dram).is_dram()) {
        pr_info("g_dram is not in main memory\n");
        return 1;
    }
    return 0;
}

declare_drv_api_main(GlobalsMain);
