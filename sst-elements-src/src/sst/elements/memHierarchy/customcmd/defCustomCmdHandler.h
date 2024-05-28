// Copyright (c) 2023 University of Washington
// Copyright 2013-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
  <mrutt@cs.washington.edu> Thu Jul 27 15:20:34 2023 -0700:
  customcmd/defCustomCmdMemory.h:
  Update constructor to new base class API.
*/
#ifndef _MEMHIERARCHY_DEFCUSTOMCMDHANDLER_H_
#define _MEMHIERARCHY_DEFCUSTOMCMDHANDLER_H_

#include <string>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

/*
 * Default subcomponent for handling custom commands at memory controller
 * Simply copies custom cmd data structure from incoming event to
 * the memory controller backend
 *
 */
class DefCustomCmdMemHandler : public CustomCmdMemHandler {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(DefCustomCmdMemHandler, "memHierarchy", "defCustomCmdHandler", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Default, custom command handler that copies custom data to backend", SST::MemHierarchy::CustomCmdMemHandler)

/* Begin class defintion */

  ////////////////////////////////////////////////////////////////////////////////////////////
  // Code here is added by UW and subject to the copyright statement at the top of the file //
  ////////////////////////////////////////////////////////////////////////////////////////////
  DefCustomCmdMemHandler(ComponentId_t id, Params &params, std::function<void(Addr,size_t,std::vector<uint8_t>&)> read, std::function<MemEventBase*(Addr,std::vector<uint8_t>*)> write, std::function<bool(Addr,size_t,std::vector<uint8_t>&,MemEventBase*)> monitor, std::function<Addr(Addr)> globalToLocal)
    : CustomCmdMemHandler(id, params, read, write, monitor, globalToLocal) {}

  ~DefCustomCmdMemHandler() {}

  CustomCmdMemHandler::MemEventInfo receive(MemEventBase* ev) override;

  Interfaces::StandardMem::CustomData* ready(MemEventBase* ev) override;

  MemEventBase* finish(MemEventBase *ev, uint32_t flags) override;

  MemEventBase* getMonitorResponse() override;

protected:
private:
};    // class DefCustomCmdMemHandler
}     // namespace MemHierarchy
}     // namespace SST

#endif
