// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

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
#include <sst/elements/memHierarchy/membackend/ramulatorBackend.h>
#include <sst/elements/memHierarchy/membackend/simpleMemBackend.h>

namespace SST {
namespace Drv {

/**
 * @brief our specialized ramulator memory backend
 *
 * this is all just to handle atomic memory operations... *sigh*
 * there HAS to be a better way to do this...
 */
class DrvRamulatorMemBackend : public SST::MemHierarchy::ramulatorMemory {
public:
  /* Element library info */
  SST_ELI_REGISTER_SUBCOMPONENT(DrvRamulatorMemBackend, "Drv", "DrvRamulatorMemBackend",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                "Custom ramulator memory backend for drv element",
                                SST::Drv::DrvRamulatorMemBackend)
  /* parameters */
  SST_ELI_DOCUMENT_PARAMS({"verbose_level", "Sets the verbosity of the backend output", "0"})

  /* constructor */
  DrvRamulatorMemBackend(ComponentId_t id, Params& params);

  /* destructor */
  ~DrvRamulatorMemBackend() override;

  bool issueCustomRequest(ReqId, Interfaces::StandardMem::CustomData*) override;

private:
  SST::Output output_;
};

} // namespace Drv
} // namespace SST
