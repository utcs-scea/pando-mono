// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvSystem.hpp"
#include "DrvCore.hpp"
#include "DrvStdMemory.hpp"
using namespace SST;
using namespace Drv;

void DrvSystem::addressToNative(DrvAPI::DrvAPIAddress address,
                                        void **native,
                                        std::size_t *size) {
    DrvStdMemory *memory = dynamic_cast<DrvStdMemory *>(core().memory_);
    if (memory == nullptr) {
        throw std::runtime_error("DrvSystem::addressToNative() requires a DrvStdMemory");
    }
    memory->toNativePointer(address, native, size);
}

uint64_t DrvSystem::getCycleCount() {
    return core().clocktc_->convertFromCoreTime(core().getCurrentSimCycle());
}

uint64_t DrvSystem::getClockHz() {
    auto period = core().clocktc_->getPeriod();
    //std::cout << "period: " << period.invert() << std::endl;
    period.invert();
    return period.getDoubleValue();
}

double DrvSystem::getSeconds() {
    return core().getElapsedSimTime().getDoubleValue();
}

void DrvSystem::outputStatistics(const std::string &tagname) {
    core().outputStatistics(tagname);
}
