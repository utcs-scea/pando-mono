// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <stdint.h>
#include <inttypes.h>

#define pr(fmt, ...)                                    \
    do {                                                \
        printf("Core %4d: Thread %4d: " fmt "",         \
               DrvAPIThread::current()->coreId(),       \
               DrvAPIThread::current()->threadId(),     \
               ##__VA_ARGS__);                          \
    } while (0)

struct foo {
    int   baz;
    float bar;
};

struct bar {
    int   obaz;
    float obar;
    float sum() const { return obaz + obar; }
};

DRV_API_REF_CLASS_BEGIN(bar)
DRV_API_REF_CLASS_DATA_MEMBER(bar,obaz)
DRV_API_REF_CLASS_DATA_MEMBER(bar,obar)
float sum() const { return obar() + obaz(); }
DRV_API_REF_CLASS_END(bar)

struct foo_ref {
public:
    foo_ref(uint64_t vaddr) : _fooptr(vaddr) {}

    DrvAPI::DrvAPIPointer<decltype(std::declval<foo>().baz)>::value_handle
    baz() { return *DrvAPI::DrvAPIPointer<int>(_fooptr.vaddr_ + offsetof(foo, baz)); }

    DrvAPI::DrvAPIPointer<decltype(std::declval<foo>().bar)>::value_handle
    bar() { return *DrvAPI::DrvAPIPointer<float>(_fooptr.vaddr_ + offsetof(foo, bar)); }

    DrvAPI::DrvAPIPointer<foo> _fooptr;
};

int PointerMain(int argc, char* argv[])
{
    using namespace DrvAPI;
    if (DrvAPIThread::current()->threadId() == 0 &&
        DrvAPIThread::current()->coreId() == 0) {
        pr("%s\n", __PRETTY_FUNCTION__);
        DrvAPIPointer<uint64_t> DRAM_BASE = DrvAPI::DrvAPIVAddress::MyL2Base().encode();
        *DRAM_BASE = 0x55;
        pr(" DRAM_BASE    = 0x%016" PRIx64 "\n", static_cast<uint64_t>(DRAM_BASE));
        pr("&DRAM_BASE[4] = 0x%016" PRIx64 "\n", static_cast<uint64_t>(&DRAM_BASE[4]));
        pr(" DRAM_BASE[0] = 0x%016" PRIx64 "\n", static_cast<uint64_t>(DRAM_BASE[0]));
        // foo_ref fptr = foo_ref(0x80000000ull);
        // fptr.baz() = 7;
        // fptr.bar() = 3.14159f;
        // pr("fptr.baz() = %d\n", static_cast<int>(fptr.baz()));
        // pr("fptr.bar() = %f\n", static_cast<float>(fptr.bar()));
        DrvAPIPointer<bar> bptr(0x80000000ull);
        bar_ref bref = &bptr[0];
        bref.obaz() = 7;
        bref.obar() = 3.14159f;
        pr("bref.obaz() = %d\n", static_cast<int>(bref.obaz()));
        pr("bref.obar() = %f\n", static_cast<float>(bref.obar()));
        pr("bref.sum()  = %f\n", bref.sum());
        // void pointer
        DrvAPIPointer<void> voidptr = DrvAPI::DrvAPIVAddress::MyL2Base().encode();
        pr("voidptr = 0x%016" PRIx64 "\n", static_cast<uint64_t>(voidptr));
    }
    return 0;
}

declare_drv_api_main(PointerMain);
