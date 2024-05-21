// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_RT_DRV_INFO_HPP_
#define PANDO_RT_DRV_INFO_HPP_

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "DrvAPIMemory.hpp"

namespace DrvAPI {
void setStageInit();
void setStageExecComp();
void setStageExecComm();
void setStageOther();
void incrementPhase();
} // namespace DrvAPI

#define PANDO_DRV_SET_STAGE_INIT() {DrvAPI::setStageInit();} 
#define PANDO_DRV_SET_STAGE_EXEC_COMP() {DrvAPI::setStageExecComp();} 
#define PANDO_DRV_SET_STAGE_EXEC_COMM() {DrvAPI::setStageExecComm();} 
#define PANDO_DRV_SET_STAGE_OTHER() {DrvAPI::setStageOther();}
#define PANDO_DRV_INCREMENT_PHASE() {DrvAPI::incrementPhase();}
#else
#define PANDO_DRV_SET_STAGE_INIT()
#define PANDO_DRV_SET_STAGE_EXEC_COMP()
#define PANDO_DRV_SET_STAGE_EXEC_COMM()
#define PANDO_DRV_SET_STAGE_OTHER()
#define PANDO_DRV_INCREMENT_PHASE()
#endif // PANDO_RT_USE_BACKEND_DRVX

#endif // PANDO_RT_DRV_INFO_HPP_