// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-rt/drv_info.hpp"

namespace DrvAPI {

stage_t program_stage = stage_t::STAGE_OTHER;

void setStageInit() {
  program_stage = stage_t::STAGE_INIT;
}
void setStageExecComp() {
  program_stage = stage_t::STAGE_EXEC_COMP;
}
void setStageExecComm() {
  program_stage = stage_t::STAGE_EXEC_COMM;
}
void setStageOther() {
  program_stage = stage_t::STAGE_OTHER;
}

} // namespace DrvAPI