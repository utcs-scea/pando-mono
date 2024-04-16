// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_RT_PROGRAM_PHASE_HPP_
#define PANDO_RT_PROGRAM_PHASE_HPP_

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "DrvAPIMemory.hpp"

namespace DrvAPI {
extern phase_t program_phase;

void phaseInitBegin();
void phaseInitEnd();
void phaseExecBegin();
void phaseExecEnd();
} // namespace pando

#define PANDO_DRV_PHASE_INIT_BEGIN() {DrvAPI::phaseInitBegin();} 
#define PANDO_DRV_PHASE_INIT_END() {DrvAPI::phaseInitEnd();}
#define PANDO_DRV_PHASE_EXEC_BEGIN() {DrvAPI::phaseExecBegin();} 
#define PANDO_DRV_PHASE_EXEC_END() {DrvAPI::phaseExecEnd();} 
#else
#define PANDO_DRV_PHASE_INIT_BEGIN()
#define PANDO_DRV_PHASE_INIT_END()
#define PANDO_DRV_PHASE_EXEC_BEGIN()
#define PANDO_DRV_PHASE_EXEC_END()
#endif // PANDO_RT_USE_BACKEND_DRVX

#endif // PANDO_RT_PROGRAM_PHASE_HPP_