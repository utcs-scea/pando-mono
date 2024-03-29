// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington

#pragma once
#include "DrvAPIReadModifyWrite.hpp"
#include "DrvAPIThreadState.hpp"
#include "DrvCustomStdMem.hpp"
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/customcmd/customCmdMemory.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEventCustom.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/membackend/dramSim3Backend.h>
#include <sst/elements/memHierarchy/membackend/simpleMemBackend.h>

namespace SST {
namespace Drv {

class DrvDRAMSim3MemBackend : public SST::MemHierarchy::DRAMSim3Memory {
public:
  SST_ELI_REGISTER_SUBCOMPONENT(DrvDRAMSim3MemBackend, "Drv", "DrvDRAMSim3MemBackend",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0), "Custom DRAMSim3 Memory Backend",
                                SST::Drv::DrvDRAMSim3MemBackend)
  SST_ELI_DOCUMENT_PARAMS({"verbose_level", "Sets the verbosity of the backend output", "0"})

  /* constructor */
  DrvDRAMSim3MemBackend(ComponentId_t id, Params& params);

  /* destructor */
  ~DrvDRAMSim3MemBackend() override;

  /* send a custom request type */
  bool issueCustomRequest(ReqId, Interfaces::StandardMem::CustomData*) override;

private:
  SST::Output output_; //!< @brief The output stream for this component
};

} // namespace Drv
} // namespace SST
