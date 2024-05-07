// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "pando-rt/drv_info.hpp"
#include <pando-rt/pando-rt.hpp>

namespace DrvAPI {
void setStageInit() {
  auto func = +[]() {
    set_stage(stage_t::STAGE_INIT);
  };
  for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
    PANDO_CHECK(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                          func));
  }
}
void setStageExecComp() {
  auto func = +[]() {
    set_stage(stage_t::STAGE_EXEC_COMP);
  };
  for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
    PANDO_CHECK(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                          func));
  }
}
void setStageExecComm() {
  auto func = +[]() {
    set_stage(stage_t::STAGE_EXEC_COMM);
  };
  for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
    PANDO_CHECK(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                          func));
  }
}
void setStageOther() {
  auto func = +[]() {
    set_stage(stage_t::STAGE_OTHER);
  };
  for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
    PANDO_CHECK(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                          func));
  }
}
void incrementPhase() {
  auto func = +[]() {
    increment_phase();
  };
  for (std::int16_t nodeId = 0; nodeId < pando::getPlaceDims().node.id; nodeId++) {
    PANDO_CHECK(
        pando::executeOn(pando::Place{pando::NodeIndex{nodeId}, pando::anyPod, pando::anyCore},
                          func));
  }
}

} // namespace DrvAPI
#endif // PANDO_RT_USE_BACKEND_DRVX