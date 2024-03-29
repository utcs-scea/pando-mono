// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <DrvEvent.hpp>

namespace SST {
namespace Drv {

/**
 * @brief The NopEvent class
 */
class DrvNopEvent : public DrvEvent {
public:
  /**
   * @brief Construct a new DrvNopEvent object
   *
   * @param _tid Thread Id.
   */
  DrvNopEvent(int _tid) : DrvEvent(), tid(_tid) {}

  int tid; //!< thread id
};

} // namespace Drv
} // namespace SST
