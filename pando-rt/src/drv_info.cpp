// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "pando-rt/drv_info.hpp"

namespace DrvAPI {
void setStageInit() {
  set_stage(stage_t::STAGE_INIT);
}
void setStageExecComp() {
  set_stage(stage_t::STAGE_EXEC_COMP);
}
void setStageExecComm() {
  set_stage(stage_t::STAGE_EXEC_COMM);
}
void setStageOther() {
  set_stage(stage_t::STAGE_OTHER);
}
void incrementPhase() {
  increment_phase();
}

} // namespace DrvAPI
#endif // PANDO_RT_USE_BACKEND_DRVX