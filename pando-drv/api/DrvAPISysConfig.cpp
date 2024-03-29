// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvAPISysConfig.hpp"
#include <atomic>

namespace DrvAPI {

static std::atomic<int> sysconfig_set;
DrvAPISysConfig DrvAPISysConfig::sysconfig;

} // namespace DrvAPI

extern "C" DrvAPI::DrvAPISysConfig* DrvAPIGetSysConfig() {
  return DrvAPI::DrvAPISysConfig::Get();
}

extern "C" void DrvAPISetSysConfig(DrvAPI::DrvAPISysConfig* sys_config) {
  // set once
  if (DrvAPI::sysconfig_set.exchange(1, std::memory_order_seq_cst) == 0) {
    DrvAPI::DrvAPISysConfig::sysconfig = *sys_config;
  }
}
