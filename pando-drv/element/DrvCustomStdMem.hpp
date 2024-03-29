// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include "DrvAPIReadModifyWrite.hpp"
#include "DrvAPIThreadState.hpp"
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/customcmd/customCmdMemory.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEventCustom.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/membackend/simpleMemBackend.h>

namespace SST {
namespace Drv {

/**
 * @brief Custom data for the atomic commands
 *
 * this is meant to be used as a member of stdMem's CustomReq
 * we create a CustomReq object and set the data to this object
 * before sending it throught the standardInterface
 */
class AtomicReqData : public Interfaces::StandardMem::CustomData {
public:
  /* constructor */
  AtomicReqData() {}

  /* return address to use for routing this event to its destination */
  Interfaces::StandardMem::Addr getRoutingAddress() override {
    return pAddr;
  }

  /* return size of to use when accounting for bandwidth used by this event */
  uint64_t getSize() override {
    return size;
  }

  /* return a CustomData* objected formatted as a response */
  Interfaces::StandardMem::CustomData* makeResponse() override {
    return this;
  }

  /* return wheter a response is needed */
  bool needsResponse() override {
    return true;
  }

  /* string representation for debugging */
  std::string getString() override {
    std::stringstream ss;
    ss << "{Type: AtomicReqData, pAddr: ";
    ss << std::hex;
    ss << pAddr << ", size: ";
    ss << std::dec;
    ss << size << "} ";
    return ss.str();
  }

  /* serialize this data for parallel sims */
  void serialize_order(SST::Core::Serialization::serializer& ser) override {
    CustomData::serialize_order(ser);
    ser& wdata;
    ser& rdata;
    ser& extdata;
    ser& size;
    ser& pAddr;
  }
  ImplementSerializable(SST::Drv::AtomicReqData);

public:
  std::vector<uint8_t> wdata;
  std::vector<uint8_t> rdata;
  std::vector<uint8_t> extdata;
  int64_t size;
  DrvAPI::DrvAPIMemAtomicType opcode;
  Interfaces::StandardMem::Addr pAddr;
};

/**
 * @brief a handler for drv custom memory operartions
 *
 * this is meant to be set as a subcomponent of memHierarchy's memoryController component
 * it handles our custom commands on the memory controller end
 */
class DrvCmdMemHandler : public SST::MemHierarchy::CustomCmdMemHandler {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_SUBCOMPONENT(DrvCmdMemHandler, "Drv", "DrvCmdMemHandler",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                "custom command handler for drv element",
                                SST::Drv::DrvCmdMemHandler)

  /* constructor
   *
   * should register a read and write handler
   */
  DrvCmdMemHandler(
      SST::ComponentId_t id, SST::Params& params,
      std::function<void(MemHierarchy::Addr, size_t, std::vector<uint8_t>&)>
          read,                                                             // read backing store
      std::function<void(MemHierarchy::Addr, std::vector<uint8_t>*)> write, // write backing store
      std::function<MemHierarchy::Addr(MemHierarchy::Addr)>
          globalToLocal); // translate global to local address

  /* destructor */
  ~DrvCmdMemHandler();

  /* Receive should decode a custom event and return an OutstandingEvent struct
   * to the memory controller so that it knows how to process the event
   */
  CustomCmdMemHandler::MemEventInfo receive(MemHierarchy::MemEventBase* ev) override;

  /* The memController will call ready when the event is ready to issue.
   * Events are ready immediately (back-to-back receive() and ready()) unless
   * the event needs to stall for some coherence action.
   * The handler should return a CustomData* which will be sent to the memBackendConvertor.
   * The memBackendConvertor will then issue the unmodified CustomCmdReq* to the backend.
   * CustomCmdReq is intended as a base class for custom commands to define as needed.
   */
  Interfaces::StandardMem::CustomData* ready(MemHierarchy::MemEventBase* ev);

  /* When the memBackendConvertor returns a response, the memController will call this function,
   * including the return flags. This function should return a response event or null if none
   * needed. It should also call the following as needed: writeData(): Update the backing store if
   * this custom command wrote data readData(): Read the backing store if the response needs data
   *  translateLocalT
   */
  MemHierarchy::MemEventBase* finish(MemHierarchy::MemEventBase* ev, uint32_t flags);

private:
  SST::Output output;
};

/**
 * @brief our specialized simple memory backend
 *
 * this is all just to handle atomic memory operations... *sigh*
 * there HAS to be a better way to do this...
 */
class DrvSimpleMemBackend : public SST::MemHierarchy::SimpleMemory {
public:
  /* Element library info */
  SST_ELI_REGISTER_SUBCOMPONENT(DrvSimpleMemBackend, "Drv", "DrvSimpleMemBackend",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                "Custom simple memory backend for drv element",
                                SST::Drv::DrvSimpleMemBackend)
  /* parameters */
  SST_ELI_DOCUMENT_PARAMS({"verbose_level", "Sets the verbosity of the backend output", "0"})

  /* constructor */
  DrvSimpleMemBackend(ComponentId_t id, Params& params);

  /* destructor */
  ~DrvSimpleMemBackend() override;

  bool issueCustomRequest(ReqId, Interfaces::StandardMem::CustomData*) override;

private:
  SST::Output output_;
};

} // namespace Drv
} // namespace SST
