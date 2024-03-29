// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <sst/core/event.h>
#include <sst/core/interfaces/stdMem.h>
#include <DrvAPIAddress.hpp>
namespace SST {
namespace Drv {

class DrvAddressMap : public SST::SubComponent {
public:
  // register this subcomponent into the element library
  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Drv::DrvAddressMap)

  // document the parameters that this component accepts
  SST_ELI_DOCUMENT_PARAMS(
      // debug flags
      {"verbose", "Verbosity of logging", "0"},
  )
  /**
   * constructor
    * @param[in] id The component id.
    * @param[in] params Parameters for this component.
    */
  DrvAddressMap(SST::ComponentId_t id, SST::Params& params);

  /**
   * destructor
   */
  virtual ~DrvAddressMap();

  /**
   * @brief Convert a virtual address to a physical address
   *
   * @param addrVirtual The virtual address
   * @return SST::Interfaces::StandardMem::Addr The physical address
   */
  virtual SST::Interfaces::StandardMem::Addr addrVirtualToPhysical(const DrvAPI::DrvAPIAddress & virti) const;

  virtual void init(unsigned int phase) {}
  virtual void setup() {}
  virtual void finish() {}

private:
  SST::Output output_; //!< @brief The output stream for this component
};

}
}
