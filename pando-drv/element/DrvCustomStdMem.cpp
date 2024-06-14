// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPIReadModifyWrite.hpp>
#include "DrvCustomStdMem.hpp"

using namespace SST;
using namespace Drv;
using namespace Interfaces;
using namespace MemHierarchy;

/* constructor
 *
 * should register a read and write handler
 */
DrvCmdMemHandler::DrvCmdMemHandler(SST::ComponentId_t id, SST::Params& params,
                                   std::function<void(Addr,size_t,std::vector<uint8_t>&)> read,
                                   std::function<MemEventBase*(Addr,std::vector<uint8_t>*)> write,
                                   std::function<bool(Addr,size_t,std::vector<uint8_t>&,bool,MemEventBase*)> monitor,
                                   std::function<Addr(MemHierarchy::Addr)> globalToLocal)
  : CustomCmdMemHandler(id, params, read, write, monitor, globalToLocal) {
  int verbose_level = params.find<int>("verbose_level", 0);
  output = SST::Output("[@f:@l:@p]: ", verbose_level, 0, SST::Output::STDOUT);
  output.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/* destructor */
DrvCmdMemHandler::~DrvCmdMemHandler() {
  output.verbose(CALL_INFO, 1, 0,"%s\n", __PRETTY_FUNCTION__);
}

/* Receive should decode a custom event and return an OutstandingEvent struct
 * to the memory controller so that it knows how to process the event
 */
CustomCmdMemHandler::MemEventInfo
DrvCmdMemHandler::receive(MemEventBase* ev) {
  output.verbose(CALL_INFO, 1, 0,"%s\n", __PRETTY_FUNCTION__);
  CustomCmdMemHandler::MemEventInfo MEI(ev->getRoutingAddress(),false);
  return MEI;
}

/* The memController will call ready when the event is ready to issue.
 * Events are ready immediately (back-to-back receive() and ready()) unless
 * the event needs to stall for some coherence action.
 * The handler should return a CustomData* which will be sent to the memBackendConvertor.
 * The memBackendConvertor will then issue the unmodified CustomCmdReq* to the backend.
 * CustomCmdReq is intended as a base class for custom commands to define as needed.
 */
Interfaces::StandardMem::CustomData*
DrvCmdMemHandler::ready(MemEventBase* ev) {
  output.verbose(CALL_INFO, 1, 0,"%s\n", __PRETTY_FUNCTION__);
  CustomMemEvent * cme = static_cast<CustomMemEvent*>(ev);
  // We don't need to modify the data structure sent by the CPU, so just
  // pass it on to the backend
  return cme->getCustomData();
}

/* When the memBackendConvertor returns a response, the memController will call this function, including
 * the return flags. This function should return a response event or null if none needed.
 * It should also call the following as needed:
 *  writeData(): Update the backing store if this custom command wrote data
 *  monitorData(): Monitor the backing store if required
 *  readData(): Read the backing store if the response needs data
 *  translateLocalT
 */
MemEventBase*
DrvCmdMemHandler::finish(MemEventBase *ev, uint32_t flags) {
  output.verbose(CALL_INFO, 1, 0,"%s\n", __PRETTY_FUNCTION__);
  if(ev->queryFlag(MemEventBase::F_NORESPONSE)||
     ((flags & MemEventBase::F_NORESPONSE)>0)){
    // posted request
    // We need to delete the CustomData structure
    CustomMemEvent * cme = static_cast<CustomMemEvent*>(ev);
    if (cme->getCustomData() != nullptr)
      delete cme->getCustomData();
    cme->setCustomData(nullptr); // Just in case someone attempts to access it...
    return nullptr;
  }
  // get the custom data and make sure it's something we support
  CustomMemEvent * cme = static_cast<CustomMemEvent*>(ev);

  AtomicReqData *atomic_data = dynamic_cast<AtomicReqData*>(cme->getCustomData());
  if (atomic_data) {
    output.verbose(CALL_INFO, 1, 0, "Formatting response to atomic memory op\n");
    // here's where we should update backing store
    // and the response payload
    atomic_data->rdata.resize(atomic_data->getSize());
    // read value in memory
    MemHierarchy::Addr localAddr = translateGlobalToLocal(atomic_data->getRoutingAddress());
    readData(localAddr, atomic_data->getSize(), atomic_data->rdata);
    // do modify based on read value
    if (!atomic_data->extdata.empty()) {
        DrvAPI::atomic_modify
            ( &atomic_data->wdata[0]
            , &atomic_data->rdata[0]
            , &atomic_data->extdata[0]
            , &atomic_data->wdata[0]
            , atomic_data->opcode
            , atomic_data->getSize()
            );
    } else {
        DrvAPI::atomic_modify
            ( &atomic_data->wdata[0]
            , &atomic_data->rdata[0]
            , &atomic_data->wdata[0]
            , atomic_data->opcode
            , atomic_data->getSize()
            );
    }
    // write-back
    monitor_resp = writeData(localAddr, &(atomic_data->wdata));
    MemEventBase *MEB = ev->makeResponse();
    return MEB;
  }

  MonitorReqData *monitor_data = dynamic_cast<MonitorReqData*>(cme->getCustomData());
  if (monitor_data) {
    output.verbose(CALL_INFO, 1, 0, "Formatting response to monitor memory op\n");
    MemHierarchy::Addr localAddr = translateGlobalToLocal(monitor_data->getRoutingAddress());
    bool monitor_hit = monitorData(localAddr, monitor_data->getSize(), monitor_data->expected, monitor_data->equal, ev);
    monitor_resp = nullptr;
    if (monitor_hit) {
      MemEventBase *MEB = ev->makeResponse();
      return MEB;
    }
  }

  if (!(atomic_data || monitor_data)) {
    output.fatal(CALL_INFO, -1, "Error: unknown custom request type\n");
  }

  return nullptr;
}


MemEventBase*
DrvCmdMemHandler::getMonitorResponse() {
  return monitor_resp;
}


/**
 * constructor of our simple memory backend
 */
DrvSimpleMemBackend::DrvSimpleMemBackend(ComponentId_t id, Params &params)
  : SimpleMemory(id, params) {
  int verbose_level = params.find<int>("verbose_level", 0);
  output_ = SST::Output("[@f:@l:@p]: ", verbose_level, 0, SST::Output::STDOUT);
  output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}

/**
 * destructor
 */
DrvSimpleMemBackend::~DrvSimpleMemBackend() {
  output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
}


/**
 * handle custom requests for drv componenets
 */
bool
DrvSimpleMemBackend::issueCustomRequest(ReqId req_id, Interfaces::StandardMem::CustomData *data) {
  output_.verbose(CALL_INFO, 1, 0, "%s\n", __PRETTY_FUNCTION__);
  AtomicReqData *atomic_data = dynamic_cast<AtomicReqData*>(data);
  if (atomic_data) {
    output_.verbose(CALL_INFO, 1, 0, "Received atomic request\n");
    self_link->send(1, new MemCtrlEvent(req_id));
    return true;
  }

  MonitorReqData *monitor_data = dynamic_cast<MonitorReqData*>(data);
  if (monitor_data) {
    output_.verbose(CALL_INFO, 1, 0, "Received monitor request\n");
    self_link->send(1, new MemCtrlEvent(req_id));
    return true;
  }

  if (!(atomic_data || monitor_data))
    output_.fatal(CALL_INFO, -1, "Error: unknown custom request type\n");
  return false;
}
