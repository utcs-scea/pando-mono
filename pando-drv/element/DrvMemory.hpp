// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <DrvAPIAddress.hpp>
#include <DrvAPIThreadState.hpp>
#include <memory>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>

namespace SST {
namespace Drv {

class DrvCore;

/* Forward declarations */
class DrvCore;
class DrvThread;

/**
 * @brief The memory class
 *
 */
class DrvMemory : public SST::SubComponent {
public:
  // register this subcomponent into the element library
  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Drv::DrvMemory, Drv::DrvCore*)

  // document the parameters that this component accepts
  SST_ELI_DOCUMENT_PARAMS(
      // debug flags
      {"verbose", "Verbosity of logging", "0"},
      {"verbose_init", "Verbosity of logging during initialization", "0"},
      {"verbose_requests", "Verbosity of logging during request events", "0"},
      {"verbose_responses", "Verbosity of logging during response events", "0"}, )

  /**
   * @brief Construct a new DrvMemory object
   */
  DrvMemory(SST::ComponentId_t id, SST::Params& params, DrvCore* core);

  /**
   * @brief Destroy the DrvMemory object
   */
  virtual ~DrvMemory() {}

  /**
   * @brief Send a memory request
   */
  virtual void sendRequest(DrvCore* core, DrvThread* thread,
                           const std::shared_ptr<DrvAPI::DrvAPIMem>& thread_mem_req) = 0;

protected:
  SST::Output output_; //!< @brief The output stream for this component
  DrvCore* core_;      //!< @brief The core this memory is attached to
  static constexpr uint32_t VERBOSE_INIT = 0x00000001;
  static constexpr uint32_t VERBOSE_REQ = 0x00000002;
  static constexpr uint32_t VERBOSE_RSP = 0x00000004;
};

} // namespace Drv
} // namespace SST
