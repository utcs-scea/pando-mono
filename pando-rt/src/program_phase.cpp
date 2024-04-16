// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "pando-rt/program_phase.hpp"

namespace DrvAPI {

phase_t program_phase = phase_t::PHASE_OTHER;

void phaseInitBegin() {
  program_phase = phase_t::PHASE_INIT;
}
void phaseInitEnd() {
  program_phase = phase_t::PHASE_OTHER;
}
void phaseExecBegin() {
  program_phase = phase_t::PHASE_EXEC;
}
void phaseExecEnd() {
  program_phase = phase_t::PHASE_OTHER;
}

} // namespace DrvAPI