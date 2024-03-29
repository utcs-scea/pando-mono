// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include "DrvAPISystem.hpp"

namespace SST {
namespace Drv {

class DrvCore;
class DrvSystem : public DrvAPI::DrvAPISystem {
public:
  DrvSystem(DrvCore& core) : core_(core) {}
  DrvSystem(const DrvSystem& other) = delete;
  DrvSystem& operator=(const DrvSystem& other) = delete;
  DrvSystem(DrvSystem&& other) = delete;
  DrvSystem& operator=(DrvSystem&& other) = delete;
  virtual ~DrvSystem() override = default;

  DrvCore& core() {
    return core_;
  }

  virtual void addressToNative(DrvAPI::DrvAPIAddress address, void** native, std::size_t* size);

  virtual uint64_t getCycleCount() override;
  virtual uint64_t getClockHz() override;
  virtual double getSeconds() override;
  virtual void outputStatistics(const std::string& tagname) override;
  DrvCore& core_;
};

} // namespace Drv
} // namespace SST
