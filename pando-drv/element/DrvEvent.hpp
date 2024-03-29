// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>

namespace SST {
namespace Drv {

/**
 * @brief A DrvEvent is an SST::Event
 *
 */
class DrvEvent : public SST::Event {
public:
  /**
   * @brief Construct a new DrvEvent object
   */
  DrvEvent() : SST::Event() {}

  /**
   * @brief Destroy the DrvEvent object
   */
  virtual ~DrvEvent() {}

  ImplementSerializable(SST::Drv::DrvEvent);
};

} // namespace Drv
} // namespace SST
