// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_RT_PROGRAM_STAGE_HPP_
#define PANDO_RT_PROGRAM_STAGE_HPP_

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "DrvAPIThread.hpp"

namespace DrvAPI {
extern stage_t program_stage;

void stageInitBegin();
void stageInitEnd();
void stageExecBegin();
void stageExecEnd();
} // namespace DrvAPI

#define PANDO_DRV_STAGE_INIT_BEGIN() {DrvAPI::stageInitBegin();} 
#define PANDO_DRV_STAGE_INIT_END() {DrvAPI::stageInitEnd();}
#define PANDO_DRV_STAGE_EXEC_BEGIN() {DrvAPI::stageExecBegin();} 
#define PANDO_DRV_STAGE_EXEC_END() {DrvAPI::stageExecEnd();} 
#else
#define PANDO_DRV_STAGE_INIT_BEGIN()
#define PANDO_DRV_STAGE_INIT_END()
#define PANDO_DRV_STAGE_EXEC_BEGIN()
#define PANDO_DRV_STAGE_EXEC_END()
#endif // PANDO_RT_USE_BACKEND_DRVX

#endif // PANDO_RT_PROGRAM_STAGE_HPP_