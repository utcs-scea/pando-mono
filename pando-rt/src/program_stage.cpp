// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-rt/program_stage.hpp"

namespace DrvAPI {

stage_t program_stage = stage_t::STAGE_OTHER;

void stageInitBegin() {
  program_stage = stage_t::STAGE_INIT;
}
void stageInitEnd() {
  program_stage = stage_t::STAGE_OTHER;
}
void stageExecBegin() {
  program_stage = stage_t::STAGE_EXEC;
}
void stageExecEnd() {
  program_stage = stage_t::STAGE_OTHER;
}

} // namespace DrvAPI