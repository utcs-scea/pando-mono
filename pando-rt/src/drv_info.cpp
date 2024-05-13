// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "pando-rt/drv_info.hpp"
#include <pando-rt/pando-rt.hpp>

namespace DrvAPI {

void setStageInit() {
  // for the command processor
  set_stage(stage_t::STAGE_INIT);
  
  // for the working cores
  auto func = +[]() {
    set_stage(stage_t::STAGE_INIT);
  };
  auto dims = pando::getPlaceDims();
  for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    for (std::int8_t pod_x = 0; pod_x < dims.pod.x; pod_x++) {
      for (std::int8_t pod_y = 0; pod_y < dims.pod.y; pod_y++) {
        for (std::int8_t core_x = 0; core_x < dims.core.x; core_x++) {
          for (std::int8_t core_y = 0; core_y < dims.core.y; core_y++) {
            PANDO_CHECK(
                pando::executeOn(pando::Place{pando::NodeIndex{nodeId},
                                              pando::PodIndex{pod_x, pod_y},
                                              pando::CoreIndex{core_x, core_y}},
                                  func));
          }
        }
      }
    }
  }
}
void setStageExecComp() {
  // for the command processor
  set_stage(stage_t::STAGE_EXEC_COMP);
  
  // for the working cores
  auto func = +[]() {
    set_stage(stage_t::STAGE_EXEC_COMP);
  };
  auto dims = pando::getPlaceDims();
  for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    for (std::int8_t pod_x = 0; pod_x < dims.pod.x; pod_x++) {
      for (std::int8_t pod_y = 0; pod_y < dims.pod.y; pod_y++) {
        for (std::int8_t core_x = 0; core_x < dims.core.x; core_x++) {
          for (std::int8_t core_y = 0; core_y < dims.core.y; core_y++) {
            PANDO_CHECK(
                pando::executeOn(pando::Place{pando::NodeIndex{nodeId},
                                              pando::PodIndex{pod_x, pod_y},
                                              pando::CoreIndex{core_x, core_y}},
                                  func));
          }
        }
      }
    }
  }
}
void setStageExecComm() {
  // for the command processor
  set_stage(stage_t::STAGE_EXEC_COMM);
  
  // for the working cores
  auto func = +[]() {
    set_stage(stage_t::STAGE_EXEC_COMM);
  };
  auto dims = pando::getPlaceDims();
  for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    for (std::int8_t pod_x = 0; pod_x < dims.pod.x; pod_x++) {
      for (std::int8_t pod_y = 0; pod_y < dims.pod.y; pod_y++) {
        for (std::int8_t core_x = 0; core_x < dims.core.x; core_x++) {
          for (std::int8_t core_y = 0; core_y < dims.core.y; core_y++) {
            PANDO_CHECK(
                pando::executeOn(pando::Place{pando::NodeIndex{nodeId},
                                              pando::PodIndex{pod_x, pod_y},
                                              pando::CoreIndex{core_x, core_y}},
                                  func));
          }
        }
      }
    }
  }
}
void setStageOther() {
  // for the command processor
  set_stage(stage_t::STAGE_OTHER);
  
  // for the working cores
  auto func = +[]() {
    set_stage(stage_t::STAGE_OTHER);
  };
  auto dims = pando::getPlaceDims();
  for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    for (std::int8_t pod_x = 0; pod_x < dims.pod.x; pod_x++) {
      for (std::int8_t pod_y = 0; pod_y < dims.pod.y; pod_y++) {
        for (std::int8_t core_x = 0; core_x < dims.core.x; core_x++) {
          for (std::int8_t core_y = 0; core_y < dims.core.y; core_y++) {
            PANDO_CHECK(
                pando::executeOn(pando::Place{pando::NodeIndex{nodeId},
                                              pando::PodIndex{pod_x, pod_y},
                                              pando::CoreIndex{core_x, core_y}},
                                  func));
          }
        }
      }
    }
  }
}
void incrementPhase() {
  // for the command processor
  increment_phase();
  
  // for the working cores
  auto func = +[]() {
    increment_phase();
  };
  auto dims = pando::getPlaceDims();
  for (std::int64_t nodeId = 0; nodeId < dims.node.id; nodeId++) {
    for (std::int8_t pod_x = 0; pod_x < dims.pod.x; pod_x++) {
      for (std::int8_t pod_y = 0; pod_y < dims.pod.y; pod_y++) {
        for (std::int8_t core_x = 0; core_x < dims.core.x; core_x++) {
          for (std::int8_t core_y = 0; core_y < dims.core.y; core_y++) {
            PANDO_CHECK(
                pando::executeOn(pando::Place{pando::NodeIndex{nodeId},
                                              pando::PodIndex{pod_x, pod_y},
                                              pando::CoreIndex{core_x, core_y}},
                                  func));
          }
        }
      }
    }
  }
}

} // namespace DrvAPI
#endif // PANDO_RT_USE_BACKEND_DRVX