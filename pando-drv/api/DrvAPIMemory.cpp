// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPIMemory.hpp>

namespace DrvAPI
{

/**
 * @brief set the stage
 *
 */
void set_stage(stage_t stage)
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPISetStage>(stage));
    DrvAPIThread::current()->yield();
}

/**
 * @brief increment the phase
 *
 */
void increment_phase()
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIIncrementPhase>());
    DrvAPIThread::current()->yield();
}

} // namespace DrvAPI
