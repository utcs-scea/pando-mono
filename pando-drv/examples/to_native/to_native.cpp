// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>
#include <vector>

#define VERBOSE
#ifdef VERBOSE
#define pr_info(fmt, ...)                                               \
    do {                                                                \
        printf("INFO:  PXN %3d: POD: %3d: CORE %3d: " fmt ""            \
               ,myPXNId()                                               \
               ,myPodId()                                               \
               ,myCoreId()                                              \
               ,##__VA_ARGS__);                                         \
    } while (0)
#else
#define pr_info(fmt, ...)                                               \
    do {                                                                \
    } while (0)
#endif

#define pr_error(fmt, ...)                                              \
    do {                                                                \
        printf("ERROR: PXN %3d: POD: %3d: CORE %3d: " fmt ""            \
               ,myPXNId()                                               \
               ,myPodId()                                               \
               ,myCoreId()                                              \
               ,##__VA_ARGS__);                                         \
    } while (0)

int ToNativeMain(int argc, char *argv[])
{
    using namespace DrvAPI;

    std::vector<DrvAPIAddress> test_addresses = {
        DrvAPIVAddress::MyL1Base().encode(),
        DrvAPIVAddress::MyL1Base().encode() + 8,
        DrvAPIVAddress::MyL1Base().encode() + 64,
        DrvAPIVAddress::MyL1Base().encode() + 120,
        DrvAPIVAddress::MyL1Base().encode() + 128,
        DrvAPIVAddress::MyL1Base().encode() + 256,

        DrvAPIVAddress::MyL2Base().encode(),
        DrvAPIVAddress::MyL2Base().encode() + 8,
        DrvAPIVAddress::MyL2Base().encode() + 64,
        DrvAPIVAddress::MyL2Base().encode() + 120,
        DrvAPIVAddress::MyL2Base().encode() + 128,
        DrvAPIVAddress::MyL2Base().encode() + 256,

        DrvAPIVAddress::MainMemBase(myPXNId()).encode(),
        DrvAPIVAddress::MainMemBase(myPXNId()).encode() + 8,
        DrvAPIVAddress::MainMemBase(myPXNId()).encode() + 64,
        DrvAPIVAddress::MainMemBase(myPXNId()).encode() + 120,
        DrvAPIVAddress::MainMemBase(myPXNId()).encode() + 128,
        DrvAPIVAddress::MainMemBase(myPXNId()).encode() + 256,
    };

    for (auto simaddr : test_addresses) {
        DrvAPIVAddress addr = simaddr;
        pr_info("Translating %s to native pointer\n", addr.to_string().c_str());
        void *addr_native = nullptr;
        std::size_t size = 0;
        DrvAPIAddressToNative(addr.encode(), &addr_native, &size);
        pr_info("Translated to native pointer %p: size = %zu\n", addr_native, size);

        DrvAPIPointer<uint64_t> as_sim_pointer = addr.encode();
        auto *as_native_pointer = reinterpret_cast<uint64_t*>(addr_native);
        uint64_t wvalue = addr
            .to_physical(myPXNId(), myPodId(), myCoreId() >> 3, myCoreId() & 0x7)
            .encode();

        uint64_t rvalue = 0;
        pr_info("Writing %010" PRIx64 " to Simulator Address %" PRIx64"\n"
                ,wvalue
                ,(uint64_t)as_sim_pointer
                );

        *as_sim_pointer = wvalue;
        rvalue = *as_native_pointer;
        pr_info("Reading %010" PRIx64 " from Native Address %p\n"
                ,rvalue
                ,as_native_pointer
                );

        if (rvalue != wvalue) {
            pr_error("MISMATCH: Wrote %16" PRIx64 ": Read %16" PRIx64 "\n"
                    ,wvalue
                    ,rvalue
                    );
        }
    }


    return 0;
}

declare_drv_api_main(ToNativeMain);
