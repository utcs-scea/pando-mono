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


int AddrMapMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    std::string vaddrstr = "0x40000000";
    if (argc > 1) {
        vaddrstr = argv[1];
    }
    DrvAPIVAddress vaddr = std::stoll(vaddrstr, nullptr, 16);
    DrvAPIPAddress paddr = vaddr.to_physical
        (myPXNId()
        ,myPodId()
        ,myCoreY()
        ,myCoreX()
         );
    pr_info("vaddr = %s (%016" PRIx64 ")"
            ",paddr = %s (%016" PRIx64 ")\n"
            ,vaddr.to_string().c_str()
            ,vaddr.encode()
            ,paddr.to_string().c_str()
            ,paddr.encode()
            );

    return 0;
}

declare_drv_api_main(AddrMapMain);
