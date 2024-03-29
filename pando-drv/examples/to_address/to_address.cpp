// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>
#include <vector>

#define pr_info(fmt, ...)                                               \
    do {                                                                \
        printf("PXN %3d: POD: %3d: CORE %3d: " fmt ""                   \
               ,myPXNId()                                               \
               ,myPodId()                                               \
               ,myCoreId()                                              \
               ,##__VA_ARGS__);                                         \
    } while (0)


struct id_type {
    int64_t pxn;
    int64_t pod;
    int64_t core;
    int64_t thread;
};

DRV_API_REF_CLASS_BEGIN(id_type)
DRV_API_REF_CLASS_DATA_MEMBER(id_type, pxn)
DRV_API_REF_CLASS_DATA_MEMBER(id_type, pod)
DRV_API_REF_CLASS_DATA_MEMBER(id_type, core)
DRV_API_REF_CLASS_DATA_MEMBER(id_type, thread)
DRV_API_REF_CLASS_END(id_type)


int ToAddressMain(int argc, char *argv[])
{
    using namespace DrvAPI;
    struct id_type id;
    DrvAPIAddress addr = 0;
    std::size_t size = 0;
    DrvAPINativeToAddress(&id, &addr, &size);

    id_type_ref id_ref = DrvAPIPointer<id_type>(addr);

    id_ref.pxn() = myPXNId();
    id_ref.pod() = myPodId();
    id_ref.core() = myCoreId();
    id_ref.thread() = myThreadId();

    id_type *native = nullptr;
    size_t _;
    DrvAPIAddressToNative(&id_ref, (void**)&native, &_);
    if (native != &id) {
        pr_info("FAIL: AddressToNative(NativeToAddress(&id)) != &id\n");
    } else if (id.pxn != myPXNId() ||
        id.pod != myPodId() ||
        id.core != myCoreId() ||
        id.thread != myThreadId()) {
        pr_info("FAIL: id fields don't match mine\n");
    } else {
        pr_info("PASS: all checks succeeded \n");
    }

    return 0;
}

declare_drv_api_main(ToAddressMain);
